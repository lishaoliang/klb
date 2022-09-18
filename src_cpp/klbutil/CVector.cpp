// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CVector.hpp"
#include "klbmem/klb_mem.h"


namespace klb {


CVector::CVector()
{
    m_vector = klb_nvector_create();
}

CVector::~CVector()
{
    KLB_FREE_BY(m_vector, klb_nvector_destroy);
}

} // namespace klb
