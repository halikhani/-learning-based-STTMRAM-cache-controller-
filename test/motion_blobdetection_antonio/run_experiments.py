import subprocess
import sys
import os
import math
import time

JACCARD_THR = 0.5
PRECISION_THR = .90
RECALL_THR = .95

STEPS = 7
QL_MAX = 8

IMG_ID_RANGE = [227, 255]
IMG_NAME = 'frames/00XXX.bmp'
NUM_INJ_PER_TEST = 100

SEED = 0

MOTION_CMD='./motion'
MOTION_STTMRAM_CMD='./motion_sttmram'

def computeArea(coords):
  if coords[2] >= coords[0] and coords[3] >= coords[1]:
    return (coords[2]-coords[0]+1)*(coords[3]-coords[1]+1)
  return 0

def analyze_bb(bb_golden, bb_err):    
  #True positives, False positives and False negatives  
  TP=0
  FP=0
  FN=0
  
  bb_golden1 = []
  for bbg in bb_golden:
    #if there is a 1:1 match with a box, 
    #then discard the pair from the two sets and TP++.
    #otherwise add it to the golden list
    if bb_err.count(bbg) > 0:
      TP = TP + 1
      bb_err.remove(bbg)
    else:
      bb_golden1.append(bbg)
  bb_golden = bb_golden1    
    
  #cast the string of each golden bb in a list of 4 integer values
  bb_golden1 = []
  for l in bb_golden:
    bb = l.split(' ')
    bb.pop(2)
    bb_golden1.append([int(bb[0]), int(bb[1]), int(bb[2]), int(bb[3])])

  #check all jaccard indexes of all bb_err x bb_golden1
  matched_bbe = 0
  for bbe in bb_err:
    bbe = bbe.split(' ')
    bbe.pop(2)
    bbe = [int(bbe[0]), int(bbe[1]), int(bbe[2]), int(bbe[3])]
    max_jaccard = None
    max_bbg = None
    for bbg in bb_golden1:
      #compute intersection
      inters_coord=[]
      if bbe[0]>bbg[0]:
        inters_coord.append(bbe[0])
      else:
        inters_coord.append(bbg[0])
      if bbe[1]>bbg[1]:
        inters_coord.append(bbe[1])
      else:
        inters_coord.append(bbg[1])
      if bbe[2]<bbg[2]:
        inters_coord.append(bbe[2])
      else:
        inters_coord.append(bbg[2])
      if bbe[3]<bbg[3]:
        inters_coord.append(bbe[3])
      else:
        inters_coord.append(bbg[3])
      #compute jaccard index
      intersArea = computeArea(inters_coord)
      unionArea = computeArea(bbe) + computeArea(bbg) - intersArea
      jaccard = float(intersArea)/unionArea
      #compute maximum Jaccard index
      if max_jaccard == None or jaccard > max_jaccard:
        max_jaccard = jaccard
        max_bbg = bbg
    #if there is a Jaccard index and it is higher than the threshold then there is match
    if max_jaccard != None and max_jaccard >= JACCARD_THR:
      bb_golden1.remove(max_bbg)
      matched_bbe = matched_bbe + 1
  TP = TP + matched_bbe
  FP = len(bb_err) - matched_bbe
  FN = len(bb_golden1)
  return [TP, FP, FN]

if __name__ == "__main__":
  start_time = time.time()
  
  #compile 'motion_sttmram.cpp',
  compile_cmd = ['g++', 'motion_sttmram.cpp', 'cache-sim/cache.cpp', 'cache-sim/cache_model.cpp', 'cache-sim/fifo.cpp', 'cache-sim/lfu.cpp', 'cache-sim/lru.cpp', 
      'cache-sim/nru.cpp', 'cache-sim/plru.cpp', 'cache-sim/srrip.cpp', '-O3', '-std=c++11',  '-lm', '-o', MOTION_STTMRAM_CMD]
  gpp_p = subprocess.Popen(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
  errors = gpp_p.stdout.read()
  errors2 = gpp_p.stderr.read()
  ret= gpp_p.wait()
  if ret!=0:
    print "Error while compiling " + MOTION_STTMRAM_CMD
    print errors
    print errors2
    sys.exit(0)
  gpp_p = subprocess.Popen(['g++', 'motion.cpp', '-O3', '-std=c++11', '-lm', '-o', MOTION_CMD], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
  errors = gpp_p.stdout.read()
  errors2 = gpp_p.stderr.read()
  ret= gpp_p.wait()
  if ret!=0:
    print "Error while compiling " + MOTION_CMD
    print errors
    print errors2
    sys.exit(0)
  
  #execute
  log_file=open('results.txt','w')
    
  curr_config = []
  for j in range(0,STEPS):
    curr_config.append(0)
  done=False
  curr_step=STEPS-1
  
  print "CONFIG PRECISION RECALL"
  
  while not done:
    curr_conf_str = ''
    for j in curr_config:
      curr_conf_str = curr_conf_str + str(j) + '-'
    curr_conf_str = curr_conf_str[:-1]
    
    #run exps
    TP = 0
    FN = 0
    FP = 0
    for img_id in range(IMG_ID_RANGE[0], IMG_ID_RANGE[1]):
      curr_motion_cmd = [MOTION_CMD, IMG_NAME.replace("XXX", str(img_id)), IMG_NAME.replace("XXX", str(img_id+1))]
      #print curr_motion_cmd
      #run golden run
      motion_p = subprocess.Popen(curr_motion_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      bb1 = motion_p.stdout.read()  
      ret = motion_p.wait()
      if ret!=0:
        print "Nominal execution terminated with a reported problem"
        print curr_motion_cmd
        sys.exit(0)
      del motion_p
      
      bb1=bb1.split('\n')
      while bb1.count('') > 0:
        bb1.remove('')
      
      curr_motion_cmd[0]=MOTION_STTMRAM_CMD
      curr_motion_cmd.append(curr_conf_str)
      curr_motion_cmd.append(str(SEED))
      
      for exp in range(0, NUM_INJ_PER_TEST):
        #print exp
        motion_p = subprocess.Popen(curr_motion_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        bb2 = motion_p.stdout.read()
        ret = motion_p.wait()
        if ret!=0:
          if ret==-11: #there was a segfault
            bb2=''
          else:
            print "STTMRAM execution terminated with a reported problem"
            print curr_motion_cmd
            print "stderr:" + motion_p.stderr.read()
            sys.exit(0)
        del motion_p

        bb2=bb2.split('\n')
        while bb2.count('') > 0:
          bb2.remove('')

        #compute jaccard and accumulate TP, FP, FN 
        stats = analyze_bb(bb1,bb2)
        TP = TP + stats[0]
        FP = FP + stats[1]
        FN = FN + stats[2]
        log_file.write(str(curr_conf_str) + " " + str(img_id)+ " " + str(SEED) + " " + str(stats[0]) + " " + str(stats[1]) + " " + str(stats[2])+'\n')

        SEED=SEED+1
        curr_motion_cmd[4]=str(SEED)
    
    #compute precision and recall      
    precision = float(TP) / (TP+FP)
    recall = float(TP) / (TP+FN)
    print str(curr_conf_str) + " " + str(precision) + " " + str(recall)

    #accept or discard move and consequently select next configuration
    #the steps are scanned from the last one to the first one and we try to maximize the QL
    #if not possible or there is no higher QL level we move to the next step
    if precision >= PRECISION_THR and recall >= RECALL_THR: #if the move is accepted and we are at the max QL move to next step
      if curr_config[curr_step] == QL_MAX:
        curr_step = curr_step - 1
    else: #if the move is discarded, restore previous QL level and move to the next step
      curr_config[curr_step] = curr_config[curr_step] - 1
      curr_step = curr_step - 1
    if curr_step >=0: #if the current step is valid improve the QL otherwise we have finished
      curr_config[curr_step] = curr_config[curr_step] + 1  
    else:
      done = True
  
  curr_conf_str = ''    
  for j in curr_config:
    curr_conf_str = curr_conf_str + str(j) + '-'
  curr_conf_str = curr_conf_str[:-1]
  end_time = time.time()
  
  print "\nFINAL CONFIG: " + curr_conf_str
  
  exe_sec = int(end_time-start_time)
  exe_min = exe_sec / 60
  exe_sec = exe_sec % 60
  
  print "Execution time: " + str(exe_min) + "m " + str(exe_sec) + "s"
  log_file.close()
