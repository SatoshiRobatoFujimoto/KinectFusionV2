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
