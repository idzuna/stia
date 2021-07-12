@ECHO OFF

IF "%~1"=="" GOTO USAGE

SET /P H="画像の重ね合わせを何重まで許容するか指定してください（0 で無制限）"

ECHO 差分計算中...
"%~dp0scan.exe" %* > "%~dp0matrix.txt"
IF NOT %ERRORLEVEL% == 0 GOTO END

ECHO 最小サイズ計算中...
"%~dp0formulate.exe" %H% < "%~dp0matrix.txt" > "%~dp0stp.mps"
"%~dp0cbc.exe" "%~dp0stp.mps" solve solu "%~dp0sol.txt" > "%~dp0cbclog.txt"

ECHO 出力中...
"%~dp0organize.exe" -s "%~dp0sol.txt" -o "%~dp0output" "%~dp0matrix.txt"
IF NOT %ERRORLEVEL% == 0 GOTO END

ECHO 出力完了
GOTO END

:USAGE
ECHO 使い方：圧縮したい PNG ファイルをこのバッチファイルへまとめてドラッグアンドドロップしてください

:END
PAUSE
