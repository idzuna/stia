@ECHO OFF

IF "%~1"=="" GOTO USAGE

SET /P H="�摜�̏d�ˍ��킹�����d�܂ŋ��e���邩�w�肵�Ă��������i0 �Ŗ������j"

ECHO �����v�Z��...
"%~dp0scan.exe" %* > "%~dp0matrix.txt"
IF NOT %ERRORLEVEL% == 0 GOTO END

ECHO �ŏ��T�C�Y�v�Z��...
"%~dp0formulate.exe" %H% < "%~dp0matrix.txt" > "%~dp0stp.mps"
"%~dp0cbc.exe" "%~dp0stp.mps" solve solu "%~dp0sol.txt" > "%~dp0cbclog.txt"

ECHO �o�͒�...
"%~dp0organize.exe" -s "%~dp0sol.txt" -o "%~dp0output" "%~dp0matrix.txt"
IF NOT %ERRORLEVEL% == 0 GOTO END

ECHO �o�͊���
GOTO END

:USAGE
ECHO �g�����F���k������ PNG �t�@�C�������̃o�b�`�t�@�C���ւ܂Ƃ߂ăh���b�O�A���h�h���b�v���Ă�������

:END
PAUSE
