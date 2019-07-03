#!/bin/bash
###########################################################
# 
#  @author: Michael De Angelis
#  @mat: 560049
#  @project: Sistemi Operativi e Laboratorio [SOL]
#  @A.A: 2018 / 2019
# 
###########################################################


for i in {0..49}
do
	./client "usertest$i" 1 &
done
wait

for i in {0..29}
do
	./client "usertest$i" 2 &
done
	
for i in {30..49}
do
	./client "usertest$i" 3 &
done
wait
