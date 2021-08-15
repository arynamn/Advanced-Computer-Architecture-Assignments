import matplotlib.pyplot as plt 
import math
import csv

x = []
y = []

with open('addrtrace4.txt','r') as csvfile:
    plots = csv.reader(csvfile, delimiter = ',')
    for row in plots:
        x.append(float(row[0]))
        y.append(float(row[1]))

x = [math.log(xc,10) for xc in x]
#x = x[:20]
#y = y[:20]
plt.plot(x, y, color='green', linestyle='solid', linewidth = 3, 
         marker='o', markerfacecolor='blue', markersize=5) 
# setting x and y axis range 
plt.ylim(0.3,1.1) 


# naming the x axis 
plt.xlabel('Access Distance(log10)') 
# naming the y axis 
plt.ylabel('CDF') 

# giving a title to my graph 
plt.title('Part2/prog4') 
  
# function to show the plot 
plt.show()

