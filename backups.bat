rem 备份文件夹
set SOURCE_FOLDER=H:\MotoMoveTest\MotoMoveTest
set BACKUP_FOLDER=H:\MotoMoveTest
rem WinRAR 可执行文件路径
set WINRAR_PATH=C:\Program Files\WinRAR\WinRAR.exe

rem 获取当前日期和时间
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set datetime=%datetime:~0,4%-%datetime:~4,2%-%datetime:~6,2%_%datetime:~8,2%-%datetime:~10,2%

rem 构建备份文件名
set BACKUP_FILE=%BACKUP_FOLDER%\Backup\Concurrency_Backup_%datetime%.rar

echo 正在备份文件夹 %SOURCE_FOLDER% 到 %BACKUP_FILE%...

if errorlevel 1 (
    echo 备份失败！
) else (
    rem 使用 WinRAR 压缩备份文件夹
    "%WINRAR_PATH%" a -ep1  "%BACKUP_FILE%" "%SOURCE_FOLDER%\*"
    if errorlevel 1 (
        echo 压缩失败！
    ) else (
        echo 备份成功并已压缩到 %BACKUP_FILE%。
    )
)

pause