## 路由表

|           根路径        |    类型    |   备注    |
|:----------------------- |:--------- |:--------- |
| /stream/*               | 长连接     | 实时媒体流 |
| /api/v1/*               | HTTP      | v1控制协议 |
| /api/oauth2/*           | HTTP      | OAuth2.0认证 |
| /*                      | 短连接     | www静态小文件下载 |

### 实时媒体流
|             根路径          |    协议    |   备注    |
|:--------------------------- |:--------- |:--------- |
| /stream/httpflv/chnn0/sidx1 | HTTP-FLV  | 第一通道主码流 |
| /stream/wsflv/chnn0/sidx1   | WS-FLV    | 第一通道主码流 |
| /stream/mnp/chnn0/sidx1     | MNP       | 第一通道主码流 |
| /stream/httpmnp/chnn0/sidx1 | HTTP-MNP  | 第一通道主码流 |
| /stream/wsmnp/chnn0/sidx1   | WS-MNP    | 第一通道主码流 |
| /stream/rtsp/chnn0/sidx1    | RTSP      | 第一通道主码流 |
