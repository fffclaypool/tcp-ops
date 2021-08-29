## TCP(Transmission Control Protocol)
### TCPの概要

```
TCPはIPの機能を拡張して, 終点ホスト間で信頼性のある通信を提供する
コネクション指向のプロトコルであり, 通信開始時にコネクションを確立して通信終了時にはコネクションを切断する
信頼性を提供するために, 送信データの順序番号制御とそれに対する確認応答制御をする
```

### TCPヘッダとヘッダ構造体について

- ヘッダフォーマット

<img width="700" alt="スクリーンショット 2021-08-29 20 12 21" src="https://user-images.githubusercontent.com/43327056/131248406-dba191ad-6503-4a0e-8b9f-0e953c28599d.png">

| フィールド名 | ビット数 | 説明 |
| --- | --- | --- |
| 送信元ポート番号	| 16 | 送信元のポート番号の値 |
| 宛先ポート番号 | 16 | 宛先のポート番号の値 |
| シーケンス番号 | 32 | 送信したデータの順序を示す値. 「相手から受信した確認応答番号」の値 |
| 確認応答番号 | 32 | 確認応答番号の値. 「相手から受信したシーケンス番号」+「データサイズ」 |
| データオフセット	| 4 | TCPヘッダの長さを示す値 |
| 予約 |	6 | 全ビット"0"が入る. 将来の拡張のために用意されている |
| コントロールフラグ | 6 | URG、ACK、PSH、RST、SYN、FINの6つのビットで構成されている. これらのビットは"1"の値が入る（フラグを立てる）場合に意味をなす |
| ウィンドウサイズ | 16 | 受信側が一度に受信することができるデータ量を送信側に通知するために使用される. 送信側は, この値のデータ量を超えて送信することはできない |
| チェックサム | 16 | TCPヘッダとデータ部分のエラーチェックを行うために使用される値が入る |
| 緊急ポインタ | 16 | コントロールフラグのURGの値が"1"である場合にのみ使用されるフィールド. 緊急データの開始位置を示す情報が入る |
| オプション | 可変長 | TCPの通信において、性能を向上させるために利用する. 例えば, TCPコネクションの際に, MSSを決定するために使用されたりする |
| パディング | 可変長 |TCPヘッダの長さを32ビットの整数にするためにPaddingとして空データの"0"の値を入れることにより調整する |

- ヘッダ構造体

```C
/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
	u_short  th_sport;                 /* source port */
	u_short  th_dport;                 /* destination port */
	tcp_seq  th_seq;                   /* sequence number */
	tcp_seq  th_ack;                   /* acknowledgement number */
#if BYTE_ORDER == LITTLE_ENDIAN
	u_int    th_off:4,                 /* (unused) */
	         th_x2:4;                  /* data offset */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_int    th_off:4,                 /* data offset */
	         th_x2:4;                  /* (unused) */
#endif
	u_char   th_flags;
	u_short  th_win;                   /* window */
	u_short  th_sum;                   /* checksum */
	u_short  th_urp;                   /* urgent pointer */
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
};
```

- 制御フラグ

| フラグ | ビットが1のときの意味 |
| --- | --- |
| URG（Urgent Flag） | 運んでいるデータの中に緊急で処理すべきものが含まれているかどうかを意味する |
| ACK（Acknowledgement Flag） | 確認応答フィールドが有効であること意味する |
| PSH（Push Flag） | 受信したデータをOS内でバッファリングせずに, 速やかにアプリケーションに渡すように指示する |
| RST（Reset Flag） | コネクションを強制的に切断することを意味する |
| SYN（Synchronize Flag） | コネクション確立要求を意味する |
| FIN（Fin Flag） | コネクション切断要求を意味する |

- 状態遷移図

<img width="750" alt="スクリーンショット 2021-08-28 19 58 32" src="https://user-images.githubusercontent.com/43327056/131242365-b3c27eb6-90f4-404d-bc29-13a3fb157f42.png">

### Reference
- [TCP/IPネットワーク実験プログラミング](https://www.amazon.co.jp/dp/B08BR75XYY)
- https://www.infraexpert.com/study/tcpip8.html
