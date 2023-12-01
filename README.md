# JetsonCamApp

FLIRカメラからSpinnnaker SDKを用いて画像取得し、所望の処理を経てデータを保存するJetson nano用QtベースのGUIアプリです。

This is a Qt-based GUI application for the Jetson nano that acquires images from a FLIR camera using the Spinnaker SDK and stores the data after the desired processing.

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
qmake
make
```

アーキテクチャごとに ```Makefile``` は異なります。このアプリに固有の設定はSpinnaker SDKのインストールディレクトリです。エラーが出る場合は ```build.pro``` を編集してください 。

## アプリの実行 Execute the app 

アプリ実行の前にSpinViewでカメラ設定を行ってください。カメラの設定は内部に保存されます。保存された設定はカメラの電源が切れるとリセットされます。