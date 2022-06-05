// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/CListen.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>

namespace klb {
    CListen::CListen(klb_multiplex_t* p_multi)
    {
        m_listen = klb_listen_create(p_multi);
    }

    CListen::CListen(CMultiplex& multi)
    {
        m_listen = klb_listen_create(multi.GetMultiplex());
    }

    CListen::~CListen()
    {
        Close();
        KLB_FREE_BY(m_listen, klb_listen_destroy);
    }

    klb_listen_t* CListen::GetListen()
    {
        return m_listen;
    }

    int CListen::SetAccept(klb_listen_accept_cb cb_accept, void* ptr)
    {
        return klb_listen_set_accept(m_listen, cb_accept, ptr);
    }

    int CListen::Open(int port, int max_connect)
    {
        return klb_listen_open(m_listen, port, max_connect);
    }

    void CListen::Close()
    {
        klb_listen_close(m_listen);
    }

    int CListen::Port()
    {
        return klb_listen_port(m_listen);
    }
}
