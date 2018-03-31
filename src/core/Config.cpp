/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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

#include <string.h>
#include <uv.h>


#include "core/Config.h"
#include "core/ConfigCreator.h"
#include "core/ConfigLoader.h"
#include "Cpu.h"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "xmrig.h"


xmrig::Config::Config() : xmrig::CommonConfig(),
    m_doubleHash(false),
    m_dryRun(false),
    m_hugePages(true),
    m_safe(false),
    m_algoVariant(0),
    m_maxCpuUsage(75),
    m_printTime(60),
    m_priority(-1),
    m_threads(0),
    m_affinity(-1L)
{

}


xmrig::Config::~Config()
{
}


bool xmrig::Config::reload(const char *json)
{
    return xmrig::ConfigLoader::reload(this, json);
}


void xmrig::Config::getJSON(rapidjson::Document &doc) const
{
    doc.SetObject();

//    auto &allocator = doc.GetAllocator();

//    doc.AddMember("access-log-file", accessLog() ? rapidjson::Value(rapidjson::StringRef(accessLog())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
//    doc.AddMember("algo",            rapidjson::StringRef(algoName()), allocator);

//    rapidjson::Value api(rapidjson::kObjectType);
//    api.AddMember("port",         apiPort(), allocator);
//    api.AddMember("access-token", apiToken() ? rapidjson::Value(rapidjson::StringRef(apiToken())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
//    api.AddMember("worker-id",    apiWorkerId() ? rapidjson::Value(rapidjson::StringRef(apiWorkerId())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
//    api.AddMember("ipv6",         isApiIPv6(), allocator);
//    api.AddMember("restricted",   isApiRestricted(), allocator);
//    doc.AddMember("api",          api, allocator);

//    doc.AddMember("background",   isBackground(), allocator);

//    rapidjson::Value bind(rapidjson::kArrayType);
//    for (const Addr *addr : m_addrs) {
//        bind.PushBack(rapidjson::StringRef(addr->addr()), allocator);
//    }

//    doc.AddMember("bind",         bind, allocator);
//    doc.AddMember("colors",       isColors(), allocator);
//    doc.AddMember("custom-diff",  diff(), allocator);
//    doc.AddMember("donate-level", donateLevel(), allocator);
//    doc.AddMember("log-file",     logFile() ? rapidjson::Value(rapidjson::StringRef(logFile())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);
//    doc.AddMember("mode",         rapidjson::StringRef(modeName()), allocator);

//    rapidjson::Value pools(rapidjson::kArrayType);

//    for (const Url *url : m_pools) {
//        rapidjson::Value obj(rapidjson::kObjectType);

//        obj.AddMember("url",     rapidjson::StringRef(url->url()), allocator);
//        obj.AddMember("user",    rapidjson::StringRef(url->user()), allocator);
//        obj.AddMember("pass",    rapidjson::StringRef(url->password()), allocator);
//        obj.AddMember("coin",    rapidjson::StringRef(url->coin()), allocator);

//        if (url->keepAlive() == 0 || url->keepAlive() == Url::kKeepAliveTimeout) {
//            obj.AddMember("keepalive", url->keepAlive() > 0, allocator);
//        }
//        else {
//            obj.AddMember("keepalive", url->keepAlive(), allocator);
//        }

//        obj.AddMember("variant", url->variant(), allocator);

//        pools.PushBack(obj, allocator);
//    }

//    doc.AddMember("pools", pools, allocator);

//    doc.AddMember("retries",       retries(), allocator);
//    doc.AddMember("retry-pause",   retryPause(), allocator);
//    doc.AddMember("reuse-timeout", reuseTimeout(), allocator);
//    doc.AddMember("user-agent",    userAgent() ? rapidjson::Value(rapidjson::StringRef(userAgent())).Move() : rapidjson::Value(rapidjson::kNullType).Move(), allocator);

//#   ifdef HAVE_SYSLOG_H
//    doc.AddMember("syslog", syslog(), allocator);
//#   endif

//    doc.AddMember("verbose",      isVerbose(), allocator);
//    doc.AddMember("watch",        m_watch,     allocator);
//    doc.AddMember("workers",      isWorkers(), allocator);
}


xmrig::Config *xmrig::Config::load(int argc, char **argv, IWatcherListener *listener)
{
    return static_cast<Config*>(ConfigLoader::load(argc, argv, new ConfigCreator(), listener));
}


bool xmrig::Config::adjust()
{
    if (!CommonConfig::adjust()) {
        return false;
    }

    m_algoVariant = getAlgoVariant();
    if (m_algoVariant == AV2_AESNI_DOUBLE || m_algoVariant == AV4_SOFT_AES_DOUBLE) {
        m_doubleHash = true;
    }

    if (!m_threads) {
        m_threads = Cpu::optimalThreadsCount(m_algorithm, m_doubleHash, m_maxCpuUsage);
    }
    else if (m_safe) {
        const int count = Cpu::optimalThreadsCount(m_algorithm, m_doubleHash, m_maxCpuUsage);
        if (m_threads > count) {
            m_threads = count;
        }
    }

    return true;
}


bool xmrig::Config::parseBoolean(int key, bool enable)
{
    if (!CommonConfig::parseBoolean(key, enable)) {
        return false;
    }

    switch (key) {
    case xmrig::IConfig::SafeKey: /* --safe */
        m_safe = enable;
        break;

    case xmrig::IConfig::HugePagesKey: /* --no-huge-pages */
        m_hugePages = enable;
        break;

    case xmrig::IConfig::DryRunKey: /* --dry-run */
        m_dryRun = enable;
        break;

    default:
        break;
    }

    return true;
}


bool xmrig::Config::parseString(int key, const char *arg)
{
    if (!CommonConfig::parseString(key, arg)) {
        return false;
    }

    switch (key) {
    case xmrig::IConfig::AVKey:          /* --av */
    case xmrig::IConfig::MaxCPUUsageKey: /* --max-cpu-usage */
    case xmrig::IConfig::CPUPriorityKey: /* --cpu-priority */
        return parseUint64(key, strtol(arg, nullptr, 10));

    case xmrig::IConfig::SafeKey:   /* --safe */
    case xmrig::IConfig::DryRunKey: /* --dry-run */
        return parseBoolean(key, true);

    case xmrig::IConfig::HugePagesKey: /* --no-huge-pages */
        return parseBoolean(key, false);

    case xmrig::IConfig::ThreadsKey:  /* --threads */
        if (strncmp(arg, "all", 3) == 0) {
            m_threads = Cpu::threads();
            return true;
        }

        return parseUint64(key, strtol(arg, nullptr, 10));

    case xmrig::IConfig::CPUAffinityKey: /* --cpu-affinity */
        {
            const char *p  = strstr(arg, "0x");
            return parseUint64(key, p ? strtoull(p, nullptr, 16) : strtoull(arg, nullptr, 10));
        }

    default:
        break;
    }

    return true;
}


bool xmrig::Config::parseUint64(int key, uint64_t arg)
{
    if (!CommonConfig::parseUint64(key, arg)) {
        return false;
    }

    switch (key) {
    case xmrig::IConfig::CPUAffinityKey: /* --cpu-affinity */
        if (arg) {
            m_affinity = arg;
        }
        break;

    default:
        return parseInt(key, static_cast<int>(arg));
    }

    return true;
}


void xmrig::Config::parseJSON(const rapidjson::Document &doc)
{
}


bool xmrig::Config::parseInt(int key, int arg)
{
    switch (key) {
    case xmrig::IConfig::ThreadsKey: /* --threads */
        if (m_threads >= 0 && arg < 1024) {
            m_threads = arg;
        }
        break;

    case xmrig::IConfig::AVKey: /* --av */
        if (arg >= AV0_AUTO && arg < AV_MAX) {
            m_algoVariant = arg;
        }
        break;

    case xmrig::IConfig::MaxCPUUsageKey: /* --max-cpu-usage */
        if (m_maxCpuUsage > 0 && arg <= 100) {
            m_maxCpuUsage = arg;
        }
        break;

    case xmrig::IConfig::CPUPriorityKey: /* --cpu-priority */
        if (arg >= 0 && arg <= 5) {
            m_priority = arg;
        }
        break;

    default:
        break;
    }

    return true;
}


int xmrig::Config::getAlgoVariant() const
{
#   ifndef XMRIG_NO_AEON
    if (m_algorithm == xmrig::ALGO_CRYPTONIGHT_LITE) {
        return getAlgoVariantLite();
    }
#   endif

    if (m_algoVariant <= AV0_AUTO || m_algoVariant >= AV_MAX) {
        return Cpu::hasAES() ? AV1_AESNI : AV3_SOFT_AES;
    }

    if (m_safe && !Cpu::hasAES() && m_algoVariant <= AV2_AESNI_DOUBLE) {
        return m_algoVariant + 2;
    }

    return m_algoVariant;
}


#ifndef XMRIG_NO_AEON
int xmrig::Config::getAlgoVariantLite() const
{
    if (m_algoVariant <= AV0_AUTO || m_algoVariant >= AV_MAX) {
        return Cpu::hasAES() ? AV2_AESNI_DOUBLE : AV4_SOFT_AES_DOUBLE;
    }

    if (m_safe && !Cpu::hasAES() && m_algoVariant <= AV2_AESNI_DOUBLE) {
        return m_algoVariant + 2;
    }

    return m_algoVariant;
}
#endif
