/**
 * 

 */
package vt_dstm;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/**
 * 
 * @author Sudhanshu
 * 
 */
public class Analyzer {

	public static void main(String[] args) {
		String directoryPath;
		int reads=0, threads=0, nodes=0;
		float throughput=0;
		
		if (args.length == 0) {
			System.err.println("Using default directory");
			directoryPath = "../ssrg-hyflow-cpp/log/";
		} else {
			directoryPath = args[0];
			System.out.println("directoryPath =" + directoryPath);
		}

		File dir = new File(directoryPath);
		FilenameFilter filter = new FilenameFilter() {
			public boolean accept(File dir, String name) {
				return name.endsWith("_result.log");
			}
		};

		String[] filenames = dir.list(filter);

		// ArrayList per Nodes count
		HashMap<Integer, HashMap<Integer, HashMap<Integer, Experiment>>> nodeMap = new HashMap<Integer, HashMap<Integer, HashMap<Integer, Experiment>>>();

		for (String file : filenames) {
			try {
				FileInputStream fstream = new FileInputStream(directoryPath+file);
				// Get the object of DataInputStream
				DataInputStream in = new DataInputStream(fstream);
				BufferedReader br = new BufferedReader(new InputStreamReader(in));
				String strLine;

				// Read File Line By Line
				while ((strLine = br.readLine()) != null) {
					// Print the content on the console
					if(strLine.startsWith("--")) {
						reads = 0;
						threads = 0;
						throughput=0;
						nodes = 0;
					}else if (strLine.startsWith("Reads")) {
						String[] values=strLine.split("=");
						values = values[1].split("%");
						String r = values[0];
						reads = Integer.parseInt(r);
					}else if (strLine.startsWith("Nodes")) {
						String[] values=strLine.split("=");
						String r = values[1];
						nodes = Integer.parseInt(r);
					}else if (strLine.startsWith("Threads")) {
						String[] values=strLine.split("=");
						threads = Integer.parseInt(values[1]);
					}else if (strLine.startsWith("Throughput")) {
						String[] values=strLine.split("=");
						throughput = Float.parseFloat(values[1]);
						HashMap<Integer, HashMap<Integer,Experiment>> readMap;
						HashMap<Integer, Experiment> throughPutMap;
						Experiment exp;
						if ( (readMap = nodeMap.get(new Integer(nodes))) == null) {
							nodeMap.put(new Integer(nodes), new HashMap<Integer, HashMap<Integer,Experiment>>());
							readMap = nodeMap.get(new Integer(nodes));
						} 
						if ( (throughPutMap = readMap.get(new Integer(reads))) == null) {
							readMap.put(new Integer(reads), new HashMap<Integer, Experiment>());
							throughPutMap = readMap.get(new Integer(reads));
						} 
						if ( (exp = throughPutMap.get(new Integer(threads))) == null) {
							throughPutMap.put(new Integer(threads), new Experiment());
							exp = throughPutMap.get(new Integer(threads));
						}
						exp.count++;
						exp.throughPut += throughput;
					}
				}
				// Close the input stream
				in.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
		List<Integer> nl= new ArrayList<Integer>(nodeMap.keySet());
		Collections.sort(nl);
		for(Integer nds : nl) {
			System.out.println("------------------------For Nodes="+nds+"----------------------");
			System.out.print(" \t"+"Threads\t"+"1\t"+"2\t"+"4\t"+"8\t"+"16\t"+"24\t"+"\n");
			HashMap<Integer, HashMap<Integer,Experiment>> readMap = nodeMap.get(new Integer(nds));
			
			List<Integer> rl = new ArrayList<Integer>(readMap.keySet());
			Collections.sort(rl);
			// Print results
			for(Integer read :rl) {
				System.out.print("Reads\t"+read.intValue()+"\t");
				HashMap<Integer, Experiment> trpMap = readMap.get(read);
				List<Integer> tl = new ArrayList<Integer>(trpMap.keySet());
				Collections.sort(tl);
				for(Integer trd: tl) {
					Experiment exp = trpMap.get(trd);
					System.out.print(nds*exp.throughPut/exp.count+"\t");
				}
				System.out.print("\n");
			}
		}
		
	}

}
