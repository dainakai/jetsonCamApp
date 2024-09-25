# JetsonCamApp

FLIRカメラからSpinnnaker SDKを用いて画像取得し、CUDAを用いた画像処理を経てデータを保存するJetson nano用QtベースのGUIアプリです。x86_64でも動作します。

## 動作、ビルド環境の構築

QtとSpinnaker SDKをインストールし、```qmake``` とC++コンパイラ, ```nvcc``` のパスが通っている状態にしてください。C++コンパイラのインストール手順はQtのインストール手順に含まれます。

## リポジトリのクローン Clone this repository

このリポジトリをクローンしてローカルに保存します。

```
git clone https://github.com/dainakai/jetsonCamApp.git
```

## ビルド Build

アプリをビルドします。

```
cd path/to/jetsonCamApp
sh ./Allclean.sh
sh ./CUDAbuild.sh
qmake
make
```

アーキテクチャごとに ```Makefile``` は異なります。このアプリに固有の設定はSpinnaker SDKのインストールディレクトリです。エラーが出る場合は ```build.pro``` を編集してください 。```Allclean.sh``` はビルドファイルや一時ファイルを削除します。

## アプリの実行 Execute the app 

アプリ実行の前にSpinViewでカメラ設定を行ってください。カメラの設定は内部に保存されます。保存された設定はカメラの電源が切れるとリセットされます。