#ifndef __KLB_CLISTEN_HPP__
#define __KLB_CLISTEN_HPP__


#include "klb_type.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbnet/CMultiplex.hpp"


namespace klb {

    KLB_EXTERN class KLB_API_CPP CListen
    {
    public:
        CListen(klb_multiplex_t* p_multi);
        CListen(CMultiplex& multi);
        ~CListen();

        klb_listen_t* GetListen();

        int SetAccept(klb_listen_accept_cb cb_accept, void* ptr);

        int Open(int port, int max_connect);
        void Close();

        int Port();

    private:
        klb_listen_t*       m_listen;
    };
}

#endif // __KLB_CLISTEN_HPP__
