import java.io.*;
import java.util.*;
import java.lang.Math;

public class vmsim{
    /*global variable initialization*/
    public static String algoType = "";
    public static int numFrames;
    public static int pageSize;
    public static int offset;
    public static int frames1;
    public static int frames2;
    public static File traceFile;
    public static int memAccess;
    public static int pageFault;
    public static int writeDisk;

    public static void main(String[] args) {
        /*Read the input*/
        if(args.length < 5 || args.length > 9){
            System.out.println("Error");
            return;
        }
        else{
            readArgs(args);
        }
        readArgs(args);

        /*Print the output*/
        if(algoType.equals("opt")){
            OPT();
            print();
        }
        else if(algoType.equals("lru")){
            LRU();
            print();
        }
        else{
            System.out.println("-Error");
        }
    }


    /*Optimal*/
    public static void OPT(){
        /*local variable initialization & trace file read line by line*/
        Scanner scan;
        Boolean dirty = true;
        Boolean dirtyRmv = true;
        int listCounter = 0; //record the line of LList
        Hashtable<Long, LinkedList<Integer>> procTable1 = new Hashtable<Long, LinkedList<Integer>>();
        Hashtable<Long, LinkedList<Integer>> procTable2 = new Hashtable<Long, LinkedList<Integer>>();
        ArrayList<Long> procLList1 = new ArrayList<Long>();
        ArrayList<Long> procLList2 = new ArrayList<Long>();
        ArrayList<Boolean> dirtyLList1 = new ArrayList<Boolean>(); //list of dirty bits
        ArrayList<Boolean> dirtyLList2 = new ArrayList<Boolean>();
        try{
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException e){
            System.out.println("No Such File");
            return;
        }
        while(scan.hasNext()) {
            memAccess++;
            String mode = (String) scan.next();
            String adrString = scan.next();
            adrString = adrString.substring(2);
            long pageAdr = Long.parseLong(adrString, 16);
            pageAdr = pageAdr >> offset; //bit shift
            int procNum = scan.nextInt();

            /*add lines to hashtable*/
            if(procNum == 0){
                if(procTable1.get(pageAdr) == null){ //if null, create a linkedlist
                    procTable1.put(pageAdr, new LinkedList<Integer>());
                    procTable1.get(pageAdr).add(listCounter);
                } else{
                    procTable1.get(pageAdr).add(listCounter);
                }
            }

            /*Same logic and similar code as Process 0*/
            if(procNum == 1){
                if(procTable2.get(pageAdr) == null){
                    procTable2.put(pageAdr, new LinkedList<Integer>());
                    procTable2.get(pageAdr).add(listCounter);
                } else{
                    procTable2.get(pageAdr).add(listCounter);
                }
            }
            listCounter++;
        }
        scan.close();

        /*re-read the tracefile */
        try{
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException e){
            System.out.println("No Such File");
            return;
        }
        while(scan.hasNext()) {
            String mode = (String) scan.next();
            String adrString = scan.next();
            adrString = adrString.substring(2);
            long pageAdr = Long.parseLong(adrString, 16);
            pageAdr = pageAdr >> offset; //bit shift
            int procNum = scan.nextInt();
            long key1;
            long key2;
            dirty = mode.equals("s"); //flag set - ‘s’ mode sets dirty bit to true

            /*normal progress*/
            if(procNum == 0){
                int index = IndexOf2(procLList1,pageAdr);
                procTable1.get(pageAdr).removeFirst();
                if(index == -1){
                    pageFault++; //add page fault for not being able to find address in page table
                    if(procLList1.size() < frames1){ //not full, add
                        procLList1.add(pageAdr);
                        dirtyLList1.add(dirty);
                    } else if(procLList1.size() == frames1){
                        key1 = optRemove(procLList1, procTable1);
                        int position1 = procLList1.indexOf(key1);
                        procLList1.remove(key1); //if full, remove head element
                        dirtyRmv = dirtyLList1.remove(position1);
                        if(dirtyRmv) //dirty so increase write disk
                            writeDisk++;
                        procLList1.add(pageAdr); //add new page and its dirty info
                        dirtyLList1.add(dirty);
                    }
                }
                else{ //index found, add to list again
                    long rmv = procLList1.remove(index);
                    dirtyRmv = dirtyLList1.remove(index);
                    dirty = dirtyRmv || dirty;
                    procLList1.add(rmv);
                    dirtyLList1.add(dirty);
                }
            }

            /*Same logic and similar code as Process 0*/
            if(procNum == 1){
                int index = IndexOf2(procLList2,pageAdr);
                procTable2.get(pageAdr).removeFirst();
                if(index == -1){
                    pageFault++;
                    if(procLList2.size() < frames2){
                        procLList2.add(pageAdr);
                        dirtyLList2.add(dirty);
                    } else if(procLList2.size() == frames2){
                        key2 = optRemove(procLList2, procTable2);
                        int position2 = procLList2.indexOf(key2);
                        procLList2.remove(key2);
                        dirtyRmv = dirtyLList2.remove(position2);
                        if(dirtyRmv)
                            writeDisk++;
                        procLList2.add(pageAdr);
                        dirtyLList2.add(dirty);
                    }
                }
                else{
                    long rmv = procLList2.remove(index);
                    dirtyRmv = dirtyLList2.remove(index);
                    dirty = dirtyRmv || dirty;
                    procLList2.add(rmv);
                    dirtyLList2.add(dirty);
                }
            }

        }
        scan.close();
    }


    /*Least Recently Used*/
    public static void LRU(){
        /*local variable initialization & trace file read line by line*/
        Scanner scan;
        Boolean dirtyRmv = true;
        Boolean dirty = true;
        LinkedList<Long> procLList1 = new LinkedList<Long>();
        LinkedList<Long> procLList2 = new LinkedList<Long>();
        LinkedList<Boolean> dirtyLList1 = new LinkedList<Boolean>(); //list of dirty bits
        LinkedList<Boolean> dirtyLList2 = new LinkedList<Boolean>();
        try{
            scan = new Scanner(traceFile);
        } catch(FileNotFoundException e){
            System.out.println("No Such File");
            return;
        }
        while(scan.hasNext()){
            memAccess++;
            String mode = (String)scan.next();
            String adrString = scan.next();
            adrString = adrString.substring(2);
            long pageAdr = Long.parseLong(adrString,16);
            pageAdr = pageAdr >> offset;
            int procNum = scan.nextInt();
            dirty = mode.equals("s"); //flag set - ‘s’ mode sets dirty bit to true

            /*normal progress*/
            if(procNum == 0){
                int index = IndexOf(procLList1,pageAdr); //call helper function to search page adr
                if(index ==-1){
                    pageFault++; //add page fault for not being able to find address in page table
                    if(procLList1.size() == frames1){
                        procLList1.remove(); //if full, remove head element
                        dirtyRmv = dirtyLList1.remove();
                        if(dirtyRmv) //dirty so increase write disk
                            writeDisk++;
                    }
                    procLList1.addLast(pageAdr); //add new page and its dirty info
                    dirtyLList1.addLast(dirty);
                }
                else{ //index found, add to list again
                    long rmv = procLList1.remove(index);
                    dirtyRmv = dirtyLList1.remove(index);
                    dirty = dirtyRmv || dirty;
                    procLList1.addLast(rmv);
                    dirtyLList1.addLast(dirty);
                }
            }

            /*Same logic and similar code as Process 0*/
            if(procNum == 1){
                int index = IndexOf(procLList2,pageAdr);
                if(index == -1){
                    pageFault++;
                    if(procLList2.size() == frames2){
                        procLList2.remove();
                        dirtyRmv = dirtyLList2.remove();
                        if(dirtyRmv)
                            writeDisk++;
                    }
                    procLList2.addLast(pageAdr);
                    dirtyLList2.addLast(dirty);
                }
                else{
                    long rmv = procLList2.remove(index);
                    dirtyRmv = dirtyLList2.remove(index);
                    dirty = dirtyRmv || dirty;
                    procLList2.addLast(rmv);
                    dirtyLList2.addLast(dirty);
                }
            }
        }
        scan.close();
    }


    /*Query whether the page table contains page adress, designed for LRU*/
    public static int IndexOf(LinkedList<Long> procLList, long pageAdr){
        for(int i = 0; i < procLList.size(); i++){
            if(procLList.get(i).longValue() == pageAdr){
                return i;
            }
        }
        return -1;
    }


    /*Query whether the page table contains page adress, designed for OPT*/
    public static int IndexOf2(ArrayList<Long> procLList, long pageAdr){
        for(int i = 0; i < procLList.size(); i++){
            if(procLList.get(i).longValue() == pageAdr){
                return i;
            }
        }
        return -1;
    }


    /*Query the element with the largest index in the list for deletion, designed for OPT*/
    public static long optRemove(ArrayList<Long> procLList, Hashtable<Long, LinkedList<Integer>> procTable) {
        int longest = 0;
        long longestIndex = 0;
        for(int i = 0; i < procLList.size(); i++){
            if(procTable.get(procLList.get(i)).size() == 0){
                return procLList.get(i);
            } else{
                if(procTable.get(procLList.get(i)).getFirst() > longest){
                    longest = procTable.get(procLList.get(i)).getFirst();
                    longestIndex = procLList.get(i);
                }
            }
        }
        return longestIndex;
    }


    /*Read command line arguments*/
    public static void readArgs(String[] args){
        algoType = args[1];
        numFrames = Integer.parseInt(args[3]);
        pageSize = Integer.parseInt(args[5]);
        offset = (int)(Math.log(pageSize*1024)/Math.log(2));
        String memSpilt[] = args[7].split(":");
        int memSpilt1 = Integer.parseInt(memSpilt[0]);
        int memSpilt2 = Integer.parseInt(memSpilt[1]);
        frames1 = (int)((double)memSpilt1 / (memSpilt1 + memSpilt2) * numFrames);
        frames2 =  (int)((double)memSpilt2 / (memSpilt1 + memSpilt2) * numFrames);
        traceFile = new File(args[8]);
    }


    /*Print the states for Gradescope*/
    public static void print(){
        System.out.println("Algorithm: " + algoType.toUpperCase());
        System.out.println("Number of frames: " + numFrames);
        System.out.println("Page size: " + pageSize + " KB");
        System.out.println("Total memory accesses: " + memAccess);
        System.out.println("Total page faults: " + pageFault);
        System.out.println("Total writes to disk: " + writeDisk);
    }
}