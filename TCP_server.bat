REM 要删除的文件路径
set "file_path=concurrency.csv"

REM 检查文件是否存在
if exist "%file_path%" (
    REM 如果文件存在，则删除文件
    del "%file_path%"
    echo 文件已删除。
) else (
    REM 如果文件不存在，则输出提示信息
    echo 文件不存在。
)

REM 等待用户按下任意键继续
pause
set SOURCE_FOLDER=H:\MotoMoveTest\MotoMoveTest
nc -l -p 12345 > MotoMoveTest.exe