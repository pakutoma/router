# router
## これはなに
苫小牧高専情報工学科 2017年度卒業研究 「IPv4・IPv6用ルーターのSoC FPGAへの実装に関する研究」にて作成したソフトウェアルーター部分

## 動作環境
Linux(IPv6に対応したカーネルが動作するLinux環境であればどこでも動作するはず)

## 機能
- IPv4
  - スタティックルーティング(デフォルトゲートウェイのみ)
  - ARP
    - リプライはLinuxのIPv4機能に依存
- IPv6
  - スタティックルーティング(デフォルトゲートウェイのみ)
  - NDP
    - リダイレクト以外
    - RAではRDNSSおよびDNSSLが使用可能
  - IPv6ホストとしての基本機能(Linuxカーネル側の機能に非依存)
    - ICMPv6 Error Message
    - ICMPv6 Infomation Message(Echo関係のみ)
    - IPv6アドレスの生成やフラグメントの分割・再結合など

# 既知の問題
- バグ
  - XMLパーサ周りにメモリリークが存在する
    - パーサを使用している部分についてはほぼXmllib2互換なので、型名を書き換えるだけで使えるはず
- 足りない機能
  - 必須対応のIPv6ヘッダ(ホップバイホップヘッダ等)に未対応

# 今後の更新予定
なし
