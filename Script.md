
---

[Manual](Manual.md) > [Script](Script.md)

---


# 概要 #
スクリプト言語として [Squirrel](http://squirrel-lang.org/) を使用しています。バージョン Squirrel 3.0 alpha 1 Unicode版 をベースにしています。 [標準ライブラリ](http://squirrel-lang.org/doc/sqstdlib2.html) のうち math と string モジュールが有効化されています。

# Squirrel に対する変更 #
Squirrel VM または標準ライブラリに対して以下の変更を行っています。

  * null に対する foreach でエラーになりません。ループを実行せずに処理を継続します。
  * format("%s") でオブジェクトを文字列に自動変換します。
  * print(...) に2個以上の引数を与えた場合、print( format(...) ) と扱われます。
  * string に以下のメソッドが追加されています。
    * startswith ( _prefix_, _start_, _end_ )
    * endswith ( _suffix_, _start_, _end_ )
    * replace ( _from_, _to_, _count_ )
    * rfind ( _substr_, _start_ )

# ライブラリ #
## グローバルシンボル ##
  * import ( filename )
    * ファイル ROOT/lib/filename.nut をロードします。
  * getslot ( _obj_ , _slot1_ , _slot2_ , ... , _slotN_ )
    * `obj[slot1][slot2]...[slotN]` を返します。途中でキーが見つからない場合は null を返します。
  * class object
    * 存在しないスロットに対するアクセスを、get\_xxx () または set\_xxx() に転送します。

## I/O ##
### class Reader ###
テキストファイルを行単位で読み込みます。行は末尾の改行を含みません。

  * constructor ( _path_ )
  * line : 現在の行です。
  * read () : 次の行を読み込みます。while ループで使用します。
  * nexti () : 次の行を読み込みます。foreach ループで使用します。

### class Writer ###
テキストファイルを書き出します。既存のファイルは上書きされます。
デフォルトのエンコーディングは UTF8 (BOM) です。

  * constructor ( _path_ , _encoding_ = "utf8" )
  * print ( _value_ ) : 文字列を書き出します。
  * write ( _value_ ) : 行を書き出します。改行が付与されます。

## シェル ##
### 関数 ###
  * os.copy ( _src_ , ... , _dst_ )
    * ファイルをコピーします。 src には複数の文字列か、文字列を含む列挙子を指定できます。 src が単数の場合、dst はファイル名またはディレクトリ名を指定します。 src が複数の場合、dst はディレクトリ名を指定します。
  * os.remove ( _src_ , ... )
    * ファイルをごみ箱に移動します。 src には複数の文字列か、文字列を含む列挙子を指定できます。
  * os.bury ( _src_ , ... )
    * ファイルを完全に削除します。 src には複数の文字列か、文字列を含む列挙子を指定できます。
  * os.rename ( _src_ , _dst_ )
    * ファイル名を変更します。
  * os.drives ()
    * 存在する利用可能なドライブをビット列として返します。 詳しくは GetLogicalDrives() を参照してください。

### class IShellItem ###
エクスプローラの項目です。作成は os.path(...) を使います。

  * id : シェル名前空間上でのパスです。
  * name : 名前です。
  * parent : 親フォルダです。
  * path : ファイルシステム上でのパスです。
  * linked : ショートカットならばリンク先です。さもなければ自身です。
  * execute ( _verb_ ) : 実行します。

## ユーザインタフェース ##

### 関数 ###
  * alert ( message , detail, caption = "Rodents" , type = INFO )
    * メッセージボックスを表示します。
  * modifiers ()
    * 修飾キーとマウスボタン。
  * menu ( fill , args )
    * ポップアップメニューを表示します。

### class Window ###
ウィンドウです。

  * constructor ( parent )
  * name : 名前
  * width : 幅
  * height : 高さ
  * location : 位置 as `[x, y]`
  * size : サイズ as `[w, h]`
  * parent : 親ウィンドウ
  * visible : 表示状態
  * close () : 閉じます。
  * cut ()
  * copy ()
  * paste ()
  * move (x, y) : ウィンドウの位置を変更します。
  * resize (w, h) : ウィンドウのサイズを変更します。
  * run () : ウィンドウが破棄されるまで待機します。

### class ListView ###
フォルダビューです。

  * constructor ( parent )

### class TreeView ###
ツリービューです。

  * constructor ( parent )

### class TextView ###
テキストエディタです。

  * constructor ( parent )
  * selection : [選択開始位置, 選択末尾位置] を取得/設定します。
  * text : テキスト

### class ReBar ###
バーコンテナです。

  * constructor ( parent )

### class ToolBar ###
ツールバーです。

  * constructor ( parent )


---

[Manual](Manual.md) > [Script](Script.md)

---