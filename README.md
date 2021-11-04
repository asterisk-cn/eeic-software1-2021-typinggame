# [week3_2](https://eeic-software1.github.io/2021/week3/#week3_2)

# ゲームで覚える英語の諺

# 解説

コンパイル：

```bash
$ gcc main.c -lm
```

実行：

```bash
$ ./a.out
```

---

## 基本操作

表示された英文を入力してください。  
正しく入力されると文字が緑色で表示され、カーソルが移動します。  
誤った入力がされると文字が赤色で表示されます。再度入力してください。

最後まで正しい入力がされると点数が加算され、次の問題が表示されます。

経過時間はプログレスバーで確認することができます。

```
Start                         Finish
+---------+---------+---------+
*******........................
```

中断するにはエンターキーを押してください。

## 点数計算

![\mbox{point} = \frac{\mbox{problem length}^{2}}{\sqrt{\mbox{time}}}](https://latex.codecogs.com/gif.latex?%5Cmbox%7Bpoint%7D%20%3D%20%5Cfrac%7B%5Cmbox%7Bproblem%20length%7D%5E%7B2%7D%7D%7B%5Csqrt%7B%5Cmbox%7Btime%7D%7D%7D)[^1] として加算します。

## 問題選択

現在のタイピング速度を評価し、それを考慮した値を平均とした正規分布を利用して生成した乱数によって、次の問題を決定します。  
問題の難易度は問題の文字数によって決定します。

## その他

ターミナルの背景が黒であることを想定して作成しています。
異なる環境では画面が見にくい場合があります。

# 推しポイント

主なタイピングゲームのようにリアルタイムで判定されるようなプログラムにしました。

操作や画面の表示が分かりやすくなるように工夫しました。

例

1. 入力されていることが分かるように入力の正誤によらずフィードバックを返す。
2. "-1.0sec"はペナルティであることが分かるように赤字で表示する。
3. コンソールへの入力で画面が煩雑にならないように入力があるたびに画面を更新する。

[^1]: point := 得点、 problem length := 問題の文字数、 time := 入力時間
