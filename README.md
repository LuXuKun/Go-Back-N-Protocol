---
title: Go-Back-N Protocol
categories: 分布式系统
tags: Networking

---
# Go-Back-N Protocol #


## Introduction: ##
本项目是半双工通信通道中Go-Back-N协议的实现及模拟应用。

## 实现原理： ##
Go-Back-N的原理简单来说就是当发送端很长时间都没有收到接收端发回的ack，或者一直接收到同一个ack时，就会重发window中缓存的packets，从而确保接收端最终正确收到己方发送的packets。
引起重发的根本原因可能是：
- 发送过去的packet中途丢失，或者接收端发回的ack中途丢失，这两种情况会导致长时间收不到ack。
- 发送过去的packet已破损，或者packets中途顺序错乱，这两种情况会导致接收端重发上一次接收到的正确packet的ack。

## Packet format: ##
|<-  1 byte  ->|<-  1 byte  ->|<-  4 byte  ->|<-             the rest            ->|
其中第一个byte标注了packet中有效data的大小，第二个byte标注了packet的type（data/ack/nak）及sequence number，
第三至第六个byte作为一个int存储了packet的checksum.

## Checksum: ##
这里采取的算法是，将packet中除存放checksum的四个byte外的数据按照每四个byte作为一个int读取（前两个byte作为short读取）
并加到checksum（初始为0）中，最终所得checksum存放在packet第三至第六个byte位。验证时只需按同样方法检查一下所得数值
是否与packet第三至六byte中所存数值相同。

## 测试方法 ##
> Run rdt_sim 1000 0.1 100 0 0 0 0 to see what happens.
> (Ruining rdt_sim without parameters will tell you the usage of this program.) In
> summary, the following are a few test cases you may want to use.
> -  rdt_sim 1000 0.1 100 0 0 0 0 there is no packet loss, corruption, or reordering in the underlying link medium.
> -  rdt_sim 1000 0.1 100 0.02 0 0 0 there is no packet loss or corruption, but there is reordering in the underlying link medium.
> -  rdt_sim 1000 0.1 100 0 0.02 0 0 there is no packet corruption or reordering, but there is packet loss in the underlying link medium.
> -  rdt_sim 1000 0.1 100 0 0 0.02 0 there is no packet loss or reordering, but there is packet corruption in the underlying link medium.
> -  rdt_sim 1000 0.1 100 0.02 0.02 0.02 0 there could be packet loss, corruption, or reordering in the underlying link medium.

## Efficiency (Window size is 5): ##

> $./rdt_sim 1000 0.1 100 0.15 0.15 0.15 0
>  Simulation completed at time 4455.07s with
> 	1005096 characters sent
> 	1005096 characters delivered
> 	79127 packets passed between the sender and the receiver
>  Congratulations! This session is error-free, loss-free, and in order.

> $./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0
>  Simulation completed at time 8233.76s with
> 	994851 characters sent
> 	994851 characters delivered
> 	109774 packets passed between the sender and the receiver
>  Congratulations! This session is error-free, loss-free, and in order.

## 缺点: ##
网络状况过于糟糕时用时较长，重发包较多