/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2019 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2018-2019 tevador     <tevador@gmail.com>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "crypto/rx/RxNUMAStorage.h"
#include "backend/common/Tags.h"
#include "backend/cpu/Cpu.h"
#include "backend/cpu/platform/HwlocCpuInfo.h"
#include "base/io/log/Log.h"
#include "base/kernel/Platform.h"
#include "base/tools/Chrono.h"
#include "base/tools/Object.h"
#include "crypto/rx/RxAlgo.h"
#include "crypto/rx/RxCache.h"
#include "crypto/rx/RxDataset.h"
#include "crypto/rx/RxSeed.h"


#include <map>
#include <mutex>
#include <hwloc.h>
#include <thread>


namespace xmrig {


constexpr size_t oneMiB = 1024 * 1024;
static std::mutex mutex;


static bool bindToNUMANode(uint32_t nodeId)
{
    auto cpu         = static_cast<HwlocCpuInfo *>(Cpu::info());
    hwloc_obj_t node = hwloc_get_numanode_obj_by_os_index(cpu->topology(), nodeId);
    if (!node) {
        return false;
    }

    if (cpu->membind(node->nodeset)) {
        Platform::setThreadAffinity(static_cast<uint64_t>(hwloc_bitmap_first(node->cpuset)));

        return true;
    }

    return false;
}


static inline void printSkipped(uint32_t nodeId, const char *reason)
{
    LOG_WARN("%s" CYAN_BOLD("#%u ") RED_BOLD("skipped") YELLOW(" (%s)"), rx_tag(), nodeId, reason);
}


static inline void printDatasetReady(uint32_t nodeId, uint64_t ts)
{
    LOG_INFO("%s" CYAN_BOLD("#%u ") GREEN_BOLD("dataset ready") BLACK_BOLD(" (%" PRIu64 " ms)"), rx_tag(), nodeId, Chrono::steadyMSecs() - ts);
}


class RxNUMAStoragePrivate
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(RxNUMAStoragePrivate)

    inline RxNUMAStoragePrivate(const std::vector<uint32_t> &nodeset) :
        m_nodeset(nodeset)
    {
        m_threads.reserve(nodeset.size());
    }


    inline ~RxNUMAStoragePrivate()
    {
        join();

        for (auto const &item : m_datasets) {
            delete item.second;
        }
    }

    inline bool isAllocated() const                     { return m_allocated; }
    inline bool isReady(const Job &job) const           { return m_ready && m_seed == job; }
    inline RxDataset *dataset(uint32_t nodeId) const    { return m_datasets.count(nodeId) ? m_datasets.at(nodeId) : m_datasets.at(m_nodeset.front()); }


    inline void setSeed(const RxSeed &seed)
    {
        m_ready = false;

        if (m_seed.algorithm() != seed.algorithm()) {
            RxAlgo::apply(seed.algorithm());
        }

        m_seed = seed;
    }


    inline void createDatasets(bool hugePages, bool oneGbPages)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        for (uint32_t node : m_nodeset) {
            m_threads.emplace_back(allocate, this, node, hugePages, oneGbPages);
        }

        join();

        std::thread thread(allocateCache, this, m_nodeset.front(), hugePages);
        thread.join();

        if (m_datasets.empty()) {
            m_datasets.insert({ m_nodeset.front(), new RxDataset(m_cache) });

            LOG_WARN(CLEAR "%s" YELLOW_BOLD_S "failed to allocate RandomX datasets, switching to slow mode" BLACK_BOLD(" (%" PRIu64 " ms)"), rx_tag(), Chrono::steadyMSecs() - ts);
        }
        else {
            dataset(m_nodeset.front())->setCache(m_cache);

            printAllocStatus(ts);
        }

        m_allocated = true;
    }


    inline void initDatasets(uint32_t threads)
    {
        uint64_t ts  = Chrono::steadyMSecs();
        auto id      = m_nodeset.front();
        auto primary = dataset(id);

        primary->init(m_seed.data(), threads);

        printDatasetReady(id, ts);

        if (m_datasets.size() > 1) {
            for (auto const &item : m_datasets) {
                if (item.first == id) {
                    continue;
                }

                m_threads.emplace_back(copyDataset, item.second, item.first, primary->raw());
            }

            join();
        }

        m_ready = true;
    }


    inline std::pair<uint32_t, uint32_t> hugePages() const
    {
        auto pages = m_cache->hugePages();
        for (auto const &item : m_datasets) {
            const auto p = item.second->hugePages(false);
            pages.first  += p.first;
            pages.second += p.second;
        }

        return pages;
    }


private:
    static void allocate(RxNUMAStoragePrivate *d_ptr, uint32_t nodeId, bool hugePages, bool oneGbPages)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        if (!bindToNUMANode(nodeId)) {
            printSkipped(nodeId, "can't bind memory");

            return;
        }

        auto dataset = new RxDataset(hugePages, oneGbPages, false, RxConfig::FastMode);
        if (!dataset->get()) {
            printSkipped(nodeId, "failed to allocate dataset");

            delete dataset;
            return;
        }

        std::lock_guard<std::mutex> lock(mutex);
        d_ptr->m_datasets.insert({ nodeId, dataset });
        d_ptr->printAllocStatus(dataset, nodeId, ts);
    }


    static void allocateCache(RxNUMAStoragePrivate *d_ptr, uint32_t nodeId, bool hugePages)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        bindToNUMANode(nodeId);

        auto cache = new RxCache(hugePages);

        std::lock_guard<std::mutex> lock(mutex);
        d_ptr->m_cache = cache;
        d_ptr->printAllocStatus(cache, nodeId, ts);
    }


    static void copyDataset(RxDataset *dst, uint32_t nodeId, const void *raw)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        dst->setRaw(raw);

        printDatasetReady(nodeId, ts);
    }


    void printAllocStatus(RxDataset *dataset, uint32_t nodeId, uint64_t ts)
    {
        const auto pages     = dataset->hugePages();
        const double percent = pages.first == 0 ? 0.0 : static_cast<double>(pages.first) / pages.second * 100.0;

        LOG_INFO("%s" CYAN_BOLD("#%u ") GREEN_BOLD("allocated") CYAN_BOLD(" %zu MB") " huge pages %s%3.0f%%" CLEAR BLACK_BOLD(" (%" PRIu64 " ms)"),
                 rx_tag(),
                 nodeId,
                 dataset->size() / oneMiB,
                 (pages.first == pages.second ? GREEN_BOLD_S : RED_BOLD_S),
                 percent,
                 Chrono::steadyMSecs() - ts
                 );
    }


    void printAllocStatus(RxCache *cache, uint32_t nodeId, uint64_t ts)
    {
        const auto pages     = cache->hugePages();
        const double percent = pages.first == 0 ? 0.0 : static_cast<double>(pages.first) / pages.second * 100.0;

        LOG_INFO("%s" CYAN_BOLD("#%u ") GREEN_BOLD("allocated") CYAN_BOLD(" %4zu MB") " huge pages %s%3.0f%%" CLEAR " %sJIT" BLACK_BOLD(" (%" PRIu64 " ms)"),
                 rx_tag(),
                 nodeId,
                 cache->size() / oneMiB,
                 (pages.first == pages.second ? GREEN_BOLD_S : RED_BOLD_S),
                 percent,
                 cache->isJIT() ? GREEN_BOLD_S "+" : RED_BOLD_S "-",
                 Chrono::steadyMSecs() - ts
                 );
    }


    void printAllocStatus(uint64_t ts)
    {
        size_t memory        = m_cache->size();
        auto pages           = hugePages();
        const double percent = pages.first == 0 ? 0.0 : static_cast<double>(pages.first) / pages.second * 100.0;

        for (auto const &item : m_datasets) {
            memory += item.second->size(false);
        }

        LOG_INFO("%s" CYAN_BOLD("-- ") GREEN_BOLD("allocated") CYAN_BOLD(" %4zu MB") " huge pages %s%3.0f%% %u/%u" CLEAR BLACK_BOLD(" (%" PRIu64 " ms)"),
                 rx_tag(),
                 memory / oneMiB,
                 (pages.first == pages.second ? GREEN_BOLD_S : (pages.first == 0 ? RED_BOLD_S : YELLOW_BOLD_S)),
                 percent,
                 pages.first,
                 pages.second,
                 Chrono::steadyMSecs() - ts
                 );
    }


    inline void join()
    {
        for (auto &thread : m_threads) {
            thread.join();
        }

        m_threads.clear();
    }


    bool m_allocated        = false;
    bool m_ready            = false;
    RxCache *m_cache        = nullptr;
    RxSeed m_seed;
    std::map<uint32_t, RxDataset *> m_datasets;
    std::vector<std::thread> m_threads;
    std::vector<uint32_t> m_nodeset;
};


} // namespace xmrig


xmrig::RxNUMAStorage::RxNUMAStorage(const std::vector<uint32_t> &nodeset) :
    d_ptr(new RxNUMAStoragePrivate(nodeset))
{
}


xmrig::RxNUMAStorage::~RxNUMAStorage()
{
    delete d_ptr;
}


xmrig::RxDataset *xmrig::RxNUMAStorage::dataset(const Job &job, uint32_t nodeId) const
{
    if (!d_ptr->isReady(job)) {
        return nullptr;
    }

    return d_ptr->dataset(nodeId);
}


std::pair<uint32_t, uint32_t> xmrig::RxNUMAStorage::hugePages() const
{
    if (!d_ptr->isAllocated()) {
        return { 0U, 0U };
    }

    return d_ptr->hugePages();
}


void xmrig::RxNUMAStorage::init(const RxSeed &seed, uint32_t threads, bool hugePages, bool oneGbPages, RxConfig::Mode)
{
    d_ptr->setSeed(seed);

    if (!d_ptr->isAllocated()) {
        d_ptr->createDatasets(hugePages, oneGbPages);
    }

    d_ptr->initDatasets(threads);
}
