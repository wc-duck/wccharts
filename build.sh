rm -rf build
mkdir build
../Qt5.7.0/5.7/gcc_64/bin/moc src/wcchart.cpp -o build/wcchart.moc.cpp
export LD_LIBRARY_PATH=../Qt5.7.0/5.7/gcc_64/lib/:$LD_LIBRARY_PATH
g++ -fPIC -std=c++11 src/wcchart.cpp -I build -I /usr/include/x86_64-linux-gnu/qt5 -Wno-deprecated-declarations -lQt5Core -lQt5Widgets -lQt5Gui -lQt5Charts -o build/wcchart

