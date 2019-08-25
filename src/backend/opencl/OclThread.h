/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
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

#ifndef XMRIG_OCLTHREAD_H
#define XMRIG_OCLTHREAD_H


#include "rapidjson/fwd.h"


namespace xmrig {


class OclThread
{
public:
    OclThread() = default;
    OclThread(uint32_t index, uint32_t intensity, uint32_t worksize, uint32_t stridedIndex, uint32_t memChunk, int64_t affinity = -1) :
        m_affinity(affinity),
        m_index(index),
        m_intensity(intensity),
        m_memChunk(memChunk),
        m_stridedIndex(stridedIndex),
        m_worksize(worksize)
    {}

    OclThread(const rapidjson::Value &value);

    inline bool isCompMode() const                          { return m_compMode; }
    inline bool isValid() const                             { return m_intensity > 0; }
    inline int64_t affinity() const                         { return m_affinity; }
    inline uint32_t bfactor() const                         { return m_bfactor; }
    inline uint32_t datasetHost() const                     { return m_datasetHost < 0 ? 0 : static_cast<uint32_t>(m_datasetHost); }
    inline uint32_t gcnAsm() const                          { return m_gcnAsm; }
    inline uint32_t index() const                           { return m_index; }
    inline uint32_t intensity() const                       { return m_intensity; }
    inline uint32_t memChunk() const                        { return m_memChunk; }
    inline uint32_t stridedIndex() const                    { return m_stridedIndex; }
    inline uint32_t unrollFactor() const                    { return m_unrollFactor; }
    inline uint32_t worksize() const                        { return m_worksize; }

    inline bool operator!=(const OclThread &other) const    { return !isEqual(other); }
    inline bool operator==(const OclThread &other) const    { return isEqual(other); }

    bool isEqual(const OclThread &other) const;
    rapidjson::Value toJSON(rapidjson::Document &doc) const;

private:
    void setUnrollFactor(uint32_t unrollFactor);

    bool m_compMode         = false;
    int m_datasetHost       = -1;
    int64_t m_affinity      = -1;
    uint32_t m_bfactor      = 6;
    uint32_t m_gcnAsm       = 1;
    uint32_t m_index        = 0;
    uint32_t m_intensity    = 0;
    uint32_t m_memChunk     = 2;
    uint32_t m_stridedIndex = 2;
    uint32_t m_unrollFactor = 8;
    uint32_t m_worksize     = 0;
};


} /* namespace xmrig */


#endif /* XMRIG_OCLTHREAD_H */
