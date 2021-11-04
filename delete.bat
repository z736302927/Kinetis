for /r E:\Code\Kinetis %a in (*.cmd) do @if exist "%a" del /s/q "%a"
for /r E:\Code\Kinetis %a in (*.o) do @if exist "%a" del /s/q "%a"
for /r E:\Code\Kinetis %a in (*.gitignore) do @if exist "%a" del /s/q "%a"
for /r E:\Code\Kinetis %a in (*.lds) do @if exist "%a" del /s/q "%a"
for /r E:\Code\Kinetis %a in (*.a) do @if exist "%a" del /s/q "%a"
for /r E:\Code\Kinetis %a in (*.tmp) do @if exist "%a" del /s/q "%a"


for /r F:\linux-5.10.49 %a in (*.cmd) do @if exist "%a" del /s/q "%a"
for /r F:\linux-5.10.49 %a in (*.o) do @if exist "%a" del /s/q "%a"
for /r F:\linux-5.10.49 %a in (*.gitignore) do @if exist "%a" del /s/q "%a"
for /r F:\linux-5.10.49 %a in (*.lds) do @if exist "%a" del /s/q "%a"
for /r F:\linux-5.10.49 %a in (*.a) do @if exist "%a" del /s/q "%a"

for /r F:\u-boot %a in (*.cmd) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.o) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.gitignore) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.lds) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.a) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.tmp) do @if exist "%a" del /s/q "%a"
for /r F:\u-boot %a in (*.su) do @if exist "%a" del /s/q "%a"


dir /b /s *.o >> o.txt
