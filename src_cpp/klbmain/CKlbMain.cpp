// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klbbase/klb_base.h"

#if defined(WIN32) && defined(DEBUG_VLD)
#include "vld.h"
#endif

// 仅用 全局的构造/析构来 初始化/退出库
class CKlbMain
{
public:
    CKlbMain() { klb_base_init(NULL); };
    ~CKlbMain() { klb_base_quit(); };
};


static CKlbMain g_klb_main;
