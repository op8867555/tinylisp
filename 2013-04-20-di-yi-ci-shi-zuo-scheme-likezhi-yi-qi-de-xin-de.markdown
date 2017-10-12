---
layout: post
title: "第一次實作scheme-like直譯器的小小心得"
date: 2013-04-20 16:16
comments: true
categories: [c, scheme, functional_programming]
---

大概花了兩個禮拜左右的時間來做，其中完成了：

- symbol (with a symbol table using pair) 
- pair (just a singly-linked list)
- lambda and closure (just copy the whole env)
- garbage collect (a simple mark-and-sweep one)
- parser 實在是不會寫，直接借[Bootstrap Scheme](http://michaux.ca/articles/scheme-from-scratch-introduction)裡頭的用

雖然只完成了這麼一點點的部份，不過也有學到東西：

- make a object structure with structs inside union 
- or use casting to simulate polymorphism.
- CGDB can't directly input, described here: 
[Sending I/O to the program being debugged](http://cgdb.sourceforge.net/docs/cgdb-no-split.html#Sending-I_002fO-to-Inferior)
- GDB can step backwardly (use record, rn, rc, rs ...etc)
- how mark-and-sweep-sweep GC works ([bootstrap Scheme](http://michaux.ca/articles/scheme-from-scratch-bootstrap-conclusion))

兩個禮拜裡，debug GC時花了三天才找出一行寫錯的單向串列…orz，
了解mark-and-sweep GC如何運作也花了好一段時間。

###往後目標###

1. 多包裝幾個getter, setter，比較好讀也比較好修改
2. 增加更多的資料型態 (ex. int, float, vector ...etc)
3. 同上，增加更多的原生方法
4. 改用function table之類的方法作dispatch (ex. print, eval ...etc)
5. 改良closure的實作，不要在複製整個環境，只抓取自由變數
6. 實作tail-recursion的優化 (參考clojure的[recur](http://clojure.org/special_forms#Special Forms))
