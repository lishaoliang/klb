// Doc Encode : UTF-8 BOM, Unix(LF)
// TLS 证书自签 (mbedtls x509write)
#include "klbnet/klb_tls_cert.h"
#include "klbmem/klb_mem.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(__KLB_MBEDTLS__)

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/x509_crt.h"

#define KLB_TLS_CERT_SAN_MAX        16
#define KLB_TLS_CERT_SAN_STR_MAX    256
#define KLB_TLS_CERT_DEFAULT_BITS   2048
#define KLB_TLS_CERT_DEFAULT_DAYS   3650

static const char* g_default_subject = "O=klb,OU=alua,CN=alua-serve";
static const char* g_default_san = "DNS:alua-serve,DNS:localhost,IP:127.0.0.1";


typedef struct san_node_klb_tls_cert_t_
{
    mbedtls_x509_san_list   node;
    char                    text[KLB_TLS_CERT_SAN_STR_MAX];
    unsigned char           ip4[4];
}san_node_klb_tls_cert_t;


static void fill_time_utc_klb_tls_cert(time_t t, char* p_out, int out_len)
{
    struct tm tm_utc;
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    // mbedtls: YYYYMMDDhhmmss
    snprintf(p_out, (size_t)out_len, "%04d%02d%02d%02d%02d%02d",
        tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
        tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
}


static int parse_ipv4_klb_tls_cert(const char* p_text, unsigned char out[4])
{
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
    unsigned int d = 0;
    int n = 0;

    if (NULL == p_text)
    {
        return -1;
    }

    n = sscanf(p_text, "%u.%u.%u.%u", &a, &b, &c, &d);
    if (4 != n)
    {
        return -1;
    }

    if (255 < a || 255 < b || 255 < c || 255 < d)
    {
        return -1;
    }

    out[0] = (unsigned char)a;
    out[1] = (unsigned char)b;
    out[2] = (unsigned char)c;
    out[3] = (unsigned char)d;
    return 0;
}


// 解析 "DNS:host,IP:1.2.3.4" 为 san 链表; 返回头指针
static mbedtls_x509_san_list* build_san_klb_tls_cert(const char* p_csv,
    san_node_klb_tls_cert_t* p_nodes, int node_cap, int* p_count)
{
    char work[1024];
    char* p_tok = NULL;
    int count = 0;
    mbedtls_x509_san_list* p_head = NULL;
    mbedtls_x509_san_list* p_prev = NULL;

    if (NULL == p_csv || NULL == p_nodes || node_cap <= 0)
    {
        return NULL;
    }

    if ((int)sizeof(work) <= (int)strlen(p_csv))
    {
        return NULL;
    }

    KLB_MEMSET(work, 0, sizeof(work));
    strncpy(work, p_csv, sizeof(work) - 1);

    p_tok = strtok(work, ",");
    while (NULL != p_tok && count < node_cap)
    {
        while (' ' == *p_tok || '\t' == *p_tok)
        {
            p_tok++;
        }

        san_node_klb_tls_cert_t* p_sn = &p_nodes[count];
        KLB_MEMSET(p_sn, 0, sizeof(*p_sn));

        if (0 == strncmp(p_tok, "DNS:", 4))
        {
            const char* p_name = p_tok + 4;
            size_t nlen = strlen(p_name);

            if (0 == nlen || KLB_TLS_CERT_SAN_STR_MAX <= nlen)
            {
                p_tok = strtok(NULL, ",");
                continue;
            }

            memcpy(p_sn->text, p_name, nlen);
            p_sn->node.node.type = MBEDTLS_X509_SAN_DNS_NAME;
            p_sn->node.node.san.unstructured_name.p = (unsigned char*)p_sn->text;
            p_sn->node.node.san.unstructured_name.len = nlen;
        }
        else if (0 == strncmp(p_tok, "IP:", 3))
        {
            if (0 != parse_ipv4_klb_tls_cert(p_tok + 3, p_sn->ip4))
            {
                p_tok = strtok(NULL, ",");
                continue;
            }

            p_sn->node.node.type = MBEDTLS_X509_SAN_IP_ADDRESS;
            p_sn->node.node.san.unstructured_name.p = p_sn->ip4;
            p_sn->node.node.san.unstructured_name.len = 4;
        }
        else
        {
            p_tok = strtok(NULL, ",");
            continue;
        }

        p_sn->node.next = NULL;
        if (NULL == p_head)
        {
            p_head = &p_sn->node;
        }
        else
        {
            p_prev->next = &p_sn->node;
        }

        p_prev = &p_sn->node;
        count++;
        p_tok = strtok(NULL, ",");
    }

    if (NULL != p_count)
    {
        *p_count = count;
    }

    return p_head;
}


static int write_file_klb_tls_cert(const char* p_path, const char* p_text, int len)
{
    FILE* fp = NULL;
    size_t n = 0;

    if (NULL == p_path || NULL == p_text || len <= 0)
    {
        return -1;
    }

    fp = fopen(p_path, "wb");
    if (NULL == fp)
    {
        return -1;
    }

    n = fwrite(p_text, 1, (size_t)len, fp);
    fclose(fp);

    if ((size_t)len != n)
    {
        return -1;
    }

    return 0;
}


bool klb_tls_cert_available(void)
{
    return true;
}


int klb_tls_cert_selfsign_rsa(const klb_tls_cert_selfsign_opt_t* p_opt,
    char* p_cert_pem, int cert_cap, int* p_cert_len,
    char* p_key_pem, int key_cap, int* p_key_len)
{
    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509write_cert crt;
    san_node_klb_tls_cert_t san_nodes[KLB_TLS_CERT_SAN_MAX];
    mbedtls_x509_san_list* p_san = NULL;
    unsigned char serial[16];
    char not_before[16];
    char not_after[16];
    const char* subject = g_default_subject;
    const char* san_csv = g_default_san;
    int bits = KLB_TLS_CERT_DEFAULT_BITS;
    int days = KLB_TLS_CERT_DEFAULT_DAYS;
    int ret = -1;
    int san_count = 0;
    time_t now = 0;
    time_t end = 0;
    size_t cert_n = 0;
    size_t key_n = 0;
    static const char pers[] = "klb_tls_cert";

    if (NULL == p_cert_pem || NULL == p_key_pem || cert_cap < KLB_TLS_CERT_PEM_MIN || key_cap < KLB_TLS_CERT_PEM_MIN)
    {
        return -1;
    }

    if (NULL != p_opt)
    {
        if (0 < p_opt->bits)
        {
            bits = p_opt->bits;
        }

        if (0 < p_opt->days)
        {
            days = p_opt->days;
        }

        if (NULL != p_opt->subject && '\0' != p_opt->subject[0])
        {
            subject = p_opt->subject;
        }

        if (NULL != p_opt->san_csv && '\0' != p_opt->san_csv[0])
        {
            san_csv = p_opt->san_csv;
        }
    }

    if (bits < 2048)
    {
        bits = 2048;
    }

    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_x509write_crt_init(&crt);

    // step1. RNG
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const unsigned char*)pers, sizeof(pers) - 1);
    if (0 != ret)
    {
        goto cleanup;
    }

    // step2. RSA key
    ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg, (unsigned int)bits, 65537);
    if (0 != ret)
    {
        goto cleanup;
    }

    // step3. validity
    now = time(NULL);
    if ((time_t)-1 == now)
    {
        now = 0;
    }

    end = now + (time_t)days * (time_t)86400;
    fill_time_utc_klb_tls_cert(now, not_before, (int)sizeof(not_before));
    fill_time_utc_klb_tls_cert(end, not_after, (int)sizeof(not_after));

    ret = mbedtls_ctr_drbg_random(&ctr_drbg, serial, sizeof(serial));
    if (0 != ret)
    {
        goto cleanup;
    }

    // 序列号最高位清零, 避免被当成负值
    serial[0] &= 0x7F;

    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);

    ret = mbedtls_x509write_crt_set_subject_name(&crt, subject);
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_issuer_name(&crt, subject);
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_serial_raw(&crt, serial, sizeof(serial));
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_validity(&crt, not_before, not_after);
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_key_usage(&crt,
        MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    if (0 != ret)
    {
        goto cleanup;
    }

    p_san = build_san_klb_tls_cert(san_csv, san_nodes, KLB_TLS_CERT_SAN_MAX, &san_count);
    if (NULL != p_san)
    {
        ret = mbedtls_x509write_crt_set_subject_alternative_name(&crt, p_san);
        if (0 != ret)
        {
            goto cleanup;
        }
    }

    // step4. PEM
    KLB_MEMSET(p_cert_pem, 0, (size_t)cert_cap);
    KLB_MEMSET(p_key_pem, 0, (size_t)key_cap);

    ret = mbedtls_x509write_crt_pem(&crt, (unsigned char*)p_cert_pem, (size_t)cert_cap,
        mbedtls_ctr_drbg_random, &ctr_drbg);
    if (0 != ret)
    {
        goto cleanup;
    }

    ret = mbedtls_pk_write_key_pem(&key, (unsigned char*)p_key_pem, (size_t)key_cap);
    if (0 != ret)
    {
        goto cleanup;
    }

    cert_n = strlen(p_cert_pem) + 1;
    key_n = strlen(p_key_pem) + 1;

    if (NULL != p_cert_len)
    {
        *p_cert_len = (int)cert_n;
    }

    if (NULL != p_key_len)
    {
        *p_key_len = (int)key_n;
    }

    ret = 0;

cleanup:
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}


int klb_tls_cert_selfsign_rsa_to_files(const klb_tls_cert_selfsign_opt_t* p_opt,
    const char* p_cert_path, const char* p_key_path)
{
    char* p_cert = NULL;
    char* p_key = NULL;
    int cert_len = 0;
    int key_len = 0;
    int rc = -1;

    if (NULL == p_cert_path || NULL == p_key_path)
    {
        return -1;
    }

    p_cert = (char*)KLB_MALLOCZ(char, KLB_TLS_CERT_PEM_MIN * 2, 0);
    p_key = (char*)KLB_MALLOCZ(char, KLB_TLS_CERT_PEM_MIN * 2, 0);

    rc = klb_tls_cert_selfsign_rsa(p_opt, p_cert, KLB_TLS_CERT_PEM_MIN * 2, &cert_len,
        p_key, KLB_TLS_CERT_PEM_MIN * 2, &key_len);
    if (0 != rc)
    {
        KLB_FREE(p_cert);
        KLB_FREE(p_key);
        return rc;
    }

    // 写出时不含强制要求的结尾 0 长度统计中的多余; 文件写 strlen
    rc = write_file_klb_tls_cert(p_cert_path, p_cert, (int)strlen(p_cert));
    if (0 != rc)
    {
        KLB_FREE(p_cert);
        KLB_FREE(p_key);
        return -1;
    }

    rc = write_file_klb_tls_cert(p_key_path, p_key, (int)strlen(p_key));
    KLB_FREE(p_cert);
    KLB_FREE(p_key);
    return rc;
}


static time_t x509_to_unix_klb_tls_cert(const mbedtls_x509_time* p_t)
{
    struct tm tm_utc;

    if (NULL == p_t)
    {
        return (time_t)-1;
    }

    KLB_MEMSET(&tm_utc, 0, sizeof(tm_utc));
    tm_utc.tm_year = p_t->year - 1900;
    tm_utc.tm_mon = p_t->mon - 1;
    tm_utc.tm_mday = p_t->day;
    tm_utc.tm_hour = p_t->hour;
    tm_utc.tm_min = p_t->min;
    tm_utc.tm_sec = p_t->sec;

#if defined(_WIN32)
    return _mkgmtime(&tm_utc);
#else
    return timegm(&tm_utc);
#endif
}


int klb_tls_cert_pem_not_after(const char* p_cert_pem, int cert_len,
    int64_t* p_not_after, int64_t* p_remaining)
{
    mbedtls_x509_crt crt;
    time_t end_t = 0;
    time_t now_t = 0;
    int rc = 0;
    size_t len = 0;

    if (NULL == p_cert_pem)
    {
        return -1;
    }

    if (cert_len <= 0)
    {
        len = strlen(p_cert_pem);
    }
    else
    {
        len = (size_t)cert_len;
    }

    if (0 == len)
    {
        return -1;
    }

    mbedtls_x509_crt_init(&crt);
    // parse 要求缓冲以 0 结尾时含结尾 0 长度, 或纯 DER; PEM 文本传 len+1
    rc = mbedtls_x509_crt_parse(&crt, (const unsigned char*)p_cert_pem, len + 1);
    if (0 != rc)
    {
        mbedtls_x509_crt_free(&crt);
        return -1;
    }

    end_t = x509_to_unix_klb_tls_cert(&crt.valid_to);
    mbedtls_x509_crt_free(&crt);

    if ((time_t)-1 == end_t)
    {
        return -1;
    }

    now_t = time(NULL);
    if (NULL != p_not_after)
    {
        *p_not_after = (int64_t)end_t;
    }

    if (NULL != p_remaining)
    {
        *p_remaining = (int64_t)end_t - (int64_t)now_t;
    }

    return 0;
}


int klb_tls_cert_file_not_after(const char* p_cert_path,
    int64_t* p_not_after, int64_t* p_remaining)
{
    FILE* fp = NULL;
    long fsz = 0;
    char* p_buf = NULL;
    size_t n = 0;
    int rc = 0;

    if (NULL == p_cert_path || '\0' == p_cert_path[0])
    {
        return -1;
    }

    fp = fopen(p_cert_path, "rb");
    if (NULL == fp)
    {
        return -1;
    }

    if (0 != fseek(fp, 0, SEEK_END))
    {
        fclose(fp);
        return -1;
    }

    fsz = ftell(fp);
    if (fsz <= 0 || fsz > 1024 * 1024)
    {
        fclose(fp);
        return -1;
    }

    if (0 != fseek(fp, 0, SEEK_SET))
    {
        fclose(fp);
        return -1;
    }

    p_buf = (char*)KLB_MALLOCZ(char, (size_t)fsz + 1, 0);
    n = fread(p_buf, 1, (size_t)fsz, fp);
    fclose(fp);

    if (n != (size_t)fsz)
    {
        KLB_FREE(p_buf);
        return -1;
    }

    p_buf[n] = '\0';
    rc = klb_tls_cert_pem_not_after(p_buf, (int)n, p_not_after, p_remaining);
    KLB_FREE(p_buf);
    return rc;
}

#else

bool klb_tls_cert_available(void)
{
    return false;
}


int klb_tls_cert_selfsign_rsa(const klb_tls_cert_selfsign_opt_t* p_opt,
    char* p_cert_pem, int cert_cap, int* p_cert_len,
    char* p_key_pem, int key_cap, int* p_key_len)
{
    (void)p_opt;
    (void)p_cert_pem;
    (void)cert_cap;
    (void)p_cert_len;
    (void)p_key_pem;
    (void)key_cap;
    (void)p_key_len;
    return -1;
}


int klb_tls_cert_selfsign_rsa_to_files(const klb_tls_cert_selfsign_opt_t* p_opt,
    const char* p_cert_path, const char* p_key_path)
{
    (void)p_opt;
    (void)p_cert_path;
    (void)p_key_path;
    return -1;
}


int klb_tls_cert_pem_not_after(const char* p_cert_pem, int cert_len,
    int64_t* p_not_after, int64_t* p_remaining)
{
    (void)p_cert_pem;
    (void)cert_len;
    (void)p_not_after;
    (void)p_remaining;
    return -1;
}


int klb_tls_cert_file_not_after(const char* p_cert_path,
    int64_t* p_not_after, int64_t* p_remaining)
{
    (void)p_cert_path;
    (void)p_not_after;
    (void)p_remaining;
    return -1;
}

#endif

// end
