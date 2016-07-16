#!/usr/bin/python

#import global_variables
import os
import sys

path_to_splitted_dirs = sys.argv[1]
path_to_splitted_dirs.strip(" /");
path_to_splitted_dirs = path_to_splitted_dirs+"/"

def quantize():
	flag=1
	present=[]
	previous=[]
	file_data=[]
	dir_list=os.listdir(path_to_splitted_dirs)
	dir_list.sort()
	#print dir_list
	no_of_files = len(dir_list)
	cur_file_no=1
	for file in dir_list:
		os.system("clear")
		print ''
		print cur_file_no,no_of_files
		print "\t\tProcessing file : "+file+" ..."
		#print str(int(cur_file_no/float(no_of_files) * 100) ) + "%"
		print ''
		print "\t\tCompleted : "+str(cur_file_no/float(no_of_files) * 100 ) + " %"
		cur_file_no = cur_file_no + 1
		os.system("sleep 0.02")
		f=open(path_to_splitted_dirs+file,'r')
		file_data=f.readlines()
		f.close()
		if flag == 1:
			for i in range(len(file_data)):
				if file_data[i].strip(' \n') == "<page>" :
					file_data[:i]=[]
					flag=0
					break	
		bottom=len(file_data)-1
		while file_data[bottom].strip(' \n') != "</page>" :
			bottom=bottom-1
		previous=present
		present=[]
		os.system("rm "+path_to_splitted_dirs+file)
	        f=open(path_to_splitted_dirs+file,'w')
		f.write("<file>"+"\n")
		for i in previous:
			f.write(i)
		for i in file_data[:bottom+1]:
			f.write(i)
		for i in file_data[bottom+1:]:
			if i[len(i)-2]=='>':
				present.append(i)
			else :
				if(i[len(i)-1]=='\n'):
					present.append(str(i[:len(i)-1]))
				else :
					present.append(i)
		f.write("</file>")
		f.close()	
		if(cur_file_no == no_of_files+1):
			print ''
			print "\n\t\t\tQuantizing files is SUCCESSFUL"
def main_quantize():
	quantize()
main_quantize()
