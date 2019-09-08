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

#ifndef XMRIG_OCLRYORUNNER_H
#define XMRIG_OCLRYORUNNER_H


#include "backend/opencl/runners/OclBaseRunner.h"


namespace xmrig {


class Cn00RyoKernel;
class Cn0Kernel;
class Cn1RyoKernel;
class Cn2RyoKernel;


class OclRyoRunner : public OclBaseRunner
{
public:
    OclRyoRunner()                          = delete;
    OclRyoRunner(const OclRyoRunner &other) = delete;
    OclRyoRunner(OclRyoRunner &&other)      = delete;
    OclRyoRunner(size_t index, const OclLaunchData &data);

    ~OclRyoRunner() override;

    OclRyoRunner &operator=(const OclRyoRunner &other) = delete;
    OclRyoRunner &operator=(OclRyoRunner &&other)      = delete;

protected:
    bool isReadyToBuild() const override;
    bool run(uint32_t nonce, uint32_t *hashOutput) override;
    bool selfTest() const override;
    bool set(const Job &job, uint8_t *blob) override;
    void build() override;

private:
    cl_mem m_scratchpads    = nullptr;
    cl_mem m_states         = nullptr;
    Cn00RyoKernel *m_cn00   = nullptr;
    Cn0Kernel *m_cn0        = nullptr;
    Cn1RyoKernel *m_cn1     = nullptr;
    Cn2RyoKernel *m_cn2     = nullptr;
};


} /* namespace xmrig */


#endif // XMRIG_OCLRYORUNNER_H
