// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/CMultiplex.hpp"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_time.h"
#include <assert.h>

namespace klb {
    CMultiplex::CMultiplex()
    {
        m_multiplex = klb_multiplex_create(klb_tick_counti64());
    }

    CMultiplex::CMultiplex(int64_t tc)
    {
        m_multiplex = klb_multiplex_create(tc);
    }

    CMultiplex::~CMultiplex()
    {
        KLB_FREE_BY(m_multiplex, klb_multiplex_destroy);
    }

    klb_multiplex_t* CMultiplex::GetMultiplex()
    {
        return m_multiplex;
    }

    int CMultiplex::PushSocket(klb_socket_t* p_socket, const klb_multiplex_ops_t* p_ops)
    {
        return klb_multiplex_push_socket(m_multiplex, p_socket, p_ops);
    }

    int CMultiplex::Remove(int id)
    {
        return klb_multiplex_remove(m_multiplex, id);
    }

    int CMultiplex::LoopOnce(int64_t now)
    {
        return klb_multiplex_loop_once(m_multiplex, now);
    }
}
