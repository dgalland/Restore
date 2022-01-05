
【warpsharp改造版について】


■warpsharp_20080325
  ・コンパイラをVC++2008に変更。VC++2008ランタイムが必要になります。（ICCは何かと不具合出ていたので廃止）
  ・aligned_allocatorに足りないコードを追加。（動作には影響ないもの。）

■warpsharp_20080101
  ・コンパイラのマイナーUPとコンパイルオプション変更

■warpsharp_20071212
  ・昔一時的に機能追加していたafsログファイル名指定（AfsLogFileという予約名称）が、MT対応Avisynthとマルチスレッド
    対応AviUtlプラグインとの間で干渉を起こしてしまっていたため削除しました。
    もう利用している人はいないと思いますが、もしどうしてもという場合は別の方法で実装を検討します。＞掲示板まで。

■warpsharp_20071206
  ・一部プラグインでAvisynth.h内の配列サイズチェックに引っかかってしまう不具合の修正。
    いつこんな不具合を埋め込んでしまったのか...元からあったというわけではないはず...

■warpsharp_20071205
  ・コンパイルオプションを修正するだけのはずが、色々気になっていたマルチスレッド処理を見直し。
    スレッドプールの構造を修正してイベントウェイトを最適化。若干高速化出来たはず。

■warpsharp_20071117
  ・色変換処理で、無駄に毎回実行されていた判定処理をスキップ＋インライン化
  ・前回分から添付していましたが、ICC版とVC版を添付。
    ICC版は/MT、VC版は/MDにてコンパイル

■warpsharp_20071108
  ・「LoadAviUtlFilterPluginMT」を「LoadAviUtlFilterPlugin」と統合。
  ・ShowAUFInfo.exeで新しい「LoadAviUtlFilterPlugin」「LoadAviUtlFilterPlugin2」の設定を出力できるように修正。
  ・thread数は、一連の設定で指定された最大値を採用するように修正。
  ※LoadAviUtlFilterPlugin2にthreadの引数がありますが、互換性のために用意しただけでマルチスレッドには対応していません。

■warpsharp_20071107（配布中止）
  ・avsfilter.dllがSSSE3モードの時落ちてしまう不具合を修正。
  ・AviUtl0.99a以降のマルチスレッドフィルタ用に「LoadAviUtlFilterPluginMT」メソッド追加。
    使い方：
        LoadAviUtlFilterPluginMT("cgcnv2.auf", "_AU_cgcnv2", copy=AviUtl_plugin_copy, debug=AviUtl_plugin_debug, thread=2)
        最後の thread が、AviUtlでのスレッド数設定と同義です。
        LoadAviUtlFilterPluginMTを複数回利用し、毎回 thread 数を変更すると、最後の数値が有効になります。
        たぶん動くと思いますが、あんまりテストしていません。

■warpsharp_20071104
  ・Intel C++ Compiler 10.0.0.27 評価版にてコンパイル。
  ・「LoadAviUtlFilterPlugin」を元に戻し、「LoadAviUtlFilterPlugin2」と名前を変更して実装。
    LoadAviUtlFilterPlugin2はメモリリーク防止になる反面、若干の速度低下あり。

■warpsharp_20071027
  ・色変換処理をSSSE3を利用して高速化。
    当然ですが、SSSE3を搭載しているCPUはCore2Duo以降になるので、そのつもりで。
    SSSE3フラグチェックは行っています。SSSE3を持っていなければ旧来のMMX(ext)で動作。

■warpsharp_20071020
  ・LoadPluginEx.dllを除いて、Intel C++ Compiler 9.1.0.28 にてコンパイル。（IDEはVC8）
  ・SSE2必須。AthlonXPでは動かないかも知れません。
  ・コンパイルオプションに/MTを利用しているため、msvcrt80.dll等は不要。
    そのため、各ファイルサイズは肥大化してます。
  ・AviUtlPluginとのデータ連携時に、若干のマージンを持たせた。
      たとえば「非線形処理による先鋭化フィルタ」など、メモリリークが原因で通常のwarpsharp
      では利用できないフィルタなどが利用可能になった。
      ※リークしても良いように画素データのおよそ倍ほどヒープ領域を確保しているため、通常版と
        比べメモリ消費量が若干あがっています。（再利用するので大した容量じゃないですが。）
  ・何箇所かの記述を、VC2005用に書き換え。あくまで文法の問題なので機能面の変更はなし。
  ・当然、過去の改良分は引継ぎ済み。（色変換、auoencマルチスレッド対応など）

