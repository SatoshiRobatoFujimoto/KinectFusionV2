# KinectFusionV2
UnaNancyOwenさんがKinect v2のサンプルプログラムを公開していらっしゃいます
- （参照コード）https://github.com/UnaNancyOwen/Kinect2Sample

# KinectFusionを引用、一部改変しました。
- SurfaceにColorを追加
- 点群ファイルの保存(.ply, .stl, .obj)
- カメラ位置を保存するプログラムを追加

# 実行の手順
- Releaseモードで実行して、's'キーで保存、'r'キーでリセット、'esc'キーで終了します。(Debugモードでウィンドウ名が文字化けするバグ有り)
- 保存されたCameraPose.ply(カメラ位置の点群)ファイルとmesh.ply(再構築した点群)ファイルをMeshLab等で開いてください。
 
# 動作環境
- PC (Windows8.1, RAM 8GB, CPU 2.7GHz)
- Visual Studio 2013
- OpenCV2.4.9
- Kinect for WIndows SDK v2.0_1409

# MIT lisence
The MIT License (MIT)

Copyright (c) 2015 Satoshi FUJIMOTO

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
