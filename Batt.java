import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.io.*;

/* This section consists of a dot that is red (discharging), blue (charging),
   or steady (dot does not appear). */
class ChargeCanvas extends Canvas {
   int charge_state;

   public ChargeCanvas () {
      setBackground(new Color(0,0,0));
      charge_state = 0;  /* 0=steady, 1=discharging, 2=charging */
   }

   public void setChargeState (int s) {
      charge_state = s;
   }

   public void paint (Graphics g) {
      Dimension d = getSize();
      int w = d.width;
      int h = d.height;
      if (charge_state == 1) {
	 g.setColor(new Color(225,0,0));
	 g.fillOval(0,0+10,(int)(3*w/4.0),(int)(3*w/4.0));
      } else if (charge_state == 2) {
	 g.setColor(new Color(0,0,225));
	 g.fillOval(0,0+10,(int)(3*w/4.0),(int)(3*w/4.0));
      }
   }
}

/* This section consists of a bar that graphically represents the percentage
   of charge remaining.  Red area represents spent charge, blue area 
   represents charge remaining */
class BattCanvas extends Canvas {
   int percent;

   public BattCanvas () {
      setBackground(new Color(0,0,255));
      percent=100;
   }

   public void setPercent (int pc) {
      percent = pc;
   }

   public void paint (Graphics g) {
      Dimension d = getSize();
      int w = d.width;
      int h = d.height;
      g.setColor(new Color(225,0,0));
      g.fillRect(0,0,w,(int)((100-percent)*(h/100.0)));
   }
}

/* This section has two textfields, one above the other, showing percentage
   charge remaining (upper) and time left in hours and minutes (lower). */
class TextCanvas extends Canvas {
   int percent, disc;  /* disc = #minutes left at current rate */

   public TextCanvas () { setBackground(new Color(0,0,0)); }

   public void setText (int pc, int mins) {  
      percent = pc;  
      disc = mins;
   }

   public String hours () {
      return String.valueOf((int)(disc/60.0));
   }

   public String minutes () {
      int res = (int)(disc % 60);
      if (res < 10) return "0"+String.valueOf(res);
      return String.valueOf(res);
   }

   public void paint (Graphics g) {
      Dimension d = getSize();
      g.setColor(new Color(0,255,0));
      String s1 = String.valueOf(percent)+"%";
      String s2 = hours()+":"+minutes();
      g.setFont(new Font("Helvetica", Font.PLAIN, 20));
      int mid = d.height/2;
      g.drawLine(0,mid,d.width,mid);
      g.drawString(s1,10,30);
      g.drawString(s2,10,60);
   }
}

/* The Applet has three sections: the discharge bar, representing the percentage
   of battery charge left; the discharge dot which is blue if charging, red
   if discharging, and not appearing if not discharging or charging; the text
   section which shows the percentage of charge remaining above the green line
   and the time remaining, below the green line.  */
class BattFrame extends JFrame {
   int charge_state, rate, rem, last_fc, power_unit;
   BattCanvas bc;
   TextCanvas tc;
   ChargeCanvas cc;

   public BattFrame () {
      Dimension d = getSize();
      setLayout(new BorderLayout());
      add ("West", bc = new BattCanvas());
      add ("East", cc = new ChargeCanvas());
      add ("Center", tc = new TextCanvas());
      bc.setSize(15,d.height);
      cc.setSize(15,d.height);
   }

   /* This function computes the values needed by all sections and 
      reports those value to those sections. 

     Two files are supplied to simulate the result of calls to _BIF 
     and _BST. Their formats are as given in the ACPI documentation.  
     File "battery_info.txt" contains information that would be extracted
     using the ACPI _BIF method - File "battery_stat.txt" contains 
     information that would be extracted using the ACPI _BST method.  

     The time left is accurate when discharging.  Nothing was done to 
     account for computing time left when charged or charging.  Perhaps
     just not showing time left in those cases is enough.   */
   public void getBattStats () {
      FileInputStream finfo = null, fstat = null;
      StringTokenizer t;
      String input;

      /* First open the files, then read from them */
      try {
	 finfo = 
	    new FileInputStream("/proc/battery_info.txt");
	 BufferedReader inf = new BufferedReader(new FileReader(finfo.getFD()));
	 fstat =
	    new FileInputStream("/proc/battery_stat.txt");
	 BufferedReader sta = new BufferedReader(new FileReader(fstat.getFD()));
	 try {
	    /* Read battery information (_BIF) for capacity */
	    input = inf.readLine();
	    t = new StringTokenizer(input, " ");
	    power_unit = Integer.parseInt(t.nextToken());
	    t.nextToken();
	    last_fc = Integer.parseInt(t.nextToken());  /* capacity is here */

	    /* Read battery status (_BST) for charge state, discharge rate,
               remaining charge */
	    input = sta.readLine();
	    t = new StringTokenizer(input, " ");
	    charge_state = Integer.parseInt(t.nextToken());
	    rate = Integer.parseInt(t.nextToken());
	    rem = Integer.parseInt(t.nextToken());

	    /* Set values in applet sections */
	    cc.setChargeState(charge_state);
	    tc.setText((rem*100)/last_fc,rem/rate);
	    bc.setPercent((rem*100)/last_fc);
	 } catch (NullPointerException r) {
	    finfo.close();
	    fstat.close();
	 }
      } catch (Exception e) {
	 System.out.println("File not found: "+e.toString());
      }
      try {
	 if (finfo != null) finfo.close();
	 if (fstat != null) fstat.close();
      } catch (Exception e) { }
   }

   /* Read the files once a second and report */
   public void doit() {
      while (true) {
	 try { Thread.sleep(1000); } catch (InterruptedException e) { }
	 getBattStats();
	 bc.repaint();
	 cc.repaint();
	 tc.repaint();
      }
   }
}

public class Batt {
   public static void main (String args[]) {
      BattFrame bf = new BattFrame();
      bf.setVisible(true);
      bf.setSize(100,100);
      bf.doit();
   }
}
