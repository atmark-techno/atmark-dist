# armsd script for vyatta
このスクリプトは vyatta用の armsd scripts ファイル集です。
vyatta 用のコマンドを用いて、SACM 上から管理することができます。

## Requirement 
 * armsd を利用できるような環境を整えておいてください

## Usage
armsd が取り扱っている scripts フォルダにコピーしてください。
 * cp * /etc/armsd/scripts/

## Description of the script
vyatta 用には、以下のスクリプトを書き換えました
 * start
   * armsd が起動した直後に LS/RS に接続をしにいくための設定
   * 今のところは、DHCP の設定しか流し込みません。 
 * reconfig
   * start スクリプトと同じ内容です。
 * command
   * vyatta の任意のオペレーションコマンドを実行できます。
   * 設定のコマンドは投入できません。

 -- Kouki Ooyatsu <kouki-o at iij.ad.jp> Fri, 27 Apr 2012
