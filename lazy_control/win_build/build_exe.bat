copy ..\lazyctrl.py lazyctrl.py
copy ..\icon_data.py icon_data.py
copy ..\icon.ico icon.ico
copy ..\startup.wav startup.wav
copy ..\theend.wav theend.wav
mkdir web
copy ..\web web

del *.exe

pyinstaller -i icon.ico --onefile --noconsole lazyctrl.py

copy "dist\lazyctrl.exe" "lazyctrl.exe"

rmdir /s /q build
rmdir /s /q dist
rmdir /s /q __pycache__
del *.spec
del *.ico
del *.py

