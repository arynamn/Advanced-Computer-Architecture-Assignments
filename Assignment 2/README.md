1.After running the programs with eight threads as mentioned in problem statement, we need to run addrtarce.cpp to collect traces of each program : <br/>
make obj-intel64/addrtrace.so <br/>
../../../pin -t obj-intel64/addrtrace.so -- ./prog1 8 <br/>

2.Put all traces generated in part I in a folder named "traces" and rename traces as addrtrace1.out, addrtrace2.out, addrtrace3.out, addrtrace4.out. To get access distance run the below command : <br/>
g++ access_dist.cpp -o access_dist <br/>
./access_dist ./traces/addrtrace1.out <br/>
This will generate a new file named "addrtrace1.out_dist.out". This file is used to plot the cumulative density function F of access distances for each of the four access traces. To plot CDF python program "plot.py" is used. <br/>
python3 plot.py

3.To pass each trace through the cache and collect the trace of accesses that miss in the cache : <br/>
 g++ cache.cpp -o cache <br/>
 ./cache ./traces/addrtrace1.out <br/>
 This will generate a new file named "addrtrace1.out_missed.out". Rename it as "addrtrace1_missed.out" and put all traces in a folder named traces_cache.Repeat the part II steps. <br/>
./access_dist ./traces_cache/addrtrace1_missed.out <br/>
This will generate a new file named "addrtrace1_missed.out_dist.out". Using this CDF is plotted.


4.Run the below command: <br/>
  g++ sharing_profile.cpp -o sprofile <br/>
 ./sprofile ./traces/addrtrace1.out <br/>
 This will generate a new file named "addrtrace1.out_sharing_profile.out" in "traces" folder which gives the sharing profile of each thread.

