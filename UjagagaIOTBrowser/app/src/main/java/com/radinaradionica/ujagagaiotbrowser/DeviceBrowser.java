package com.radinaradionica.ujagagaiotbrowser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.StrictMode;
import android.support.constraint.ConstraintLayout;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.URL;

public class DeviceBrowser extends AppCompatActivity {

    /* Configuration */
    private static final int SOCKET_TIMEOUT = 500;
    private static final int BROADCAST_PORT = 4210;         /* Port on which to broadcast the message */
    private static final String pingMsg = "ujagaga ping";   /* Message to broadcast */

    /* Global variables */
    private Handler refreshHandler;
    private Handler wifiRefreshHandler;
    private Button btnAllOff;
    private Button btnRefresh;
    private String[] devices;
    private ListView deviceList;
    private int lastClickedItemId = -1;
    private TextView statusText;
    private int wifiAvailableFlag = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_browser);

        getSupportActionBar().setBackgroundDrawable(new ColorDrawable(Color.parseColor("#1c1c1c")));

        deviceList = findViewById(R.id.list_devices);
        btnAllOff = findViewById(R.id.btn_alloff);
        btnRefresh = findViewById(R.id.btn_refresh);
        btnRefresh.setEnabled(false);
        statusText = findViewById(R.id.txt_status);

        btnAllOff.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                allDevsOff();
            }
        });

        btnRefresh.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) { refreshDevs(); }
        });

        wifiRefreshHandler = new Handler();
        WiFiStatusChecker.run();
    }

    /* Create options menu */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_menu, menu);
        return true;
    }

    /* Handle menu item click event */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if(item.getItemId() == R.id.about){
            final String aboutString = "Author: Rada Berar\ne-mail: ujagaga@gmail.com\n\n" +
                    "http://radinaradionica.com\n\nPersonal IOT project device locator via " +
                    "UDP broadcast ping.\nTip: try long click on listed device to set label.";

            final SpannableString s = new SpannableString(aboutString);
            Linkify.addLinks(s, Linkify.ALL);

            final AlertDialog alertDialog = new AlertDialog.Builder(this)
                    .setPositiveButton("Dismiss", null)
                    .setIcon(android.R.drawable.ic_dialog_info)
                    .setMessage( s )
                    .setTitle("About")
                    .create();

            alertDialog.show();

            // Make the textview clickable. Must be called after show()
            ((TextView)alertDialog.findViewById(android.R.id.message)).setMovementMethod(LinkMovementMethod.getInstance());
        }
        return true;
    }

    @Override
    public void onResume(){
        super.onResume();
        refreshDevs();
    }

    /* A delay to initiate device refresh. This way we prevent a delay to re-draw the UI */
    Runnable refreshDelayer = new Runnable() {
        @Override public void run() {
            refreshHandler.removeCallbacks(refreshDelayer);
            refreshDevs();
        }
    };

    /* Calculates current IP address or a broadcast address */
    static InetAddress getAddress(Context context, boolean broadcast) throws IOException {
        WifiManager wifi = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        DhcpInfo dhcp = wifi.getDhcpInfo();

        int addr = dhcp.ipAddress;

        byte[] quads = new byte[4];
        for (int k = 0; k < 4; k++) {
            quads[k] = (byte) ((addr >> k * 8) & 0xFF);
        }

        if(broadcast){
            quads[3] = (byte)255;
        }

        return InetAddress.getByAddress(quads);
    }

    private String queryUrl(String urlStr){
        URL url;
        HttpURLConnection urlConnection = null;
        String response = "";

        try {
            url = new URL(urlStr);

            urlConnection = (HttpURLConnection) url.openConnection();

            InputStream in = urlConnection.getInputStream();
            InputStreamReader isw = new InputStreamReader(in);

            int data = isw.read();
            while (data != -1) {
                response += (char) data;
                data = isw.read();
            }
        } catch (Exception e) {
            Log.e("ERR", "Error getting dev_id: " + e.getMessage());
        } finally {
            if (urlConnection != null) {
                urlConnection.disconnect();
            }
        }

        return response;
    }

    /* Lists all devices on the same network which respond to configured UDP ping */
    public String listDevices(){
        String result = "";
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        byte[] rcvBuffer = new byte[32];
        int i;

        /* Receive response from maximum 10 servers */
        try {
            DatagramSocket socket = new DatagramSocket();
            socket.setBroadcast(true);
            socket.setSoTimeout(SOCKET_TIMEOUT);

            byte[] sendData = pingMsg.getBytes();

            try {
                DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, getAddress(this, true), BROADCAST_PORT);
                socket.send(sendPacket);
//                Log.i("MSG", "Broadcast packet sent to: " + getAddress(this, true).getHostAddress());
            } catch (IOException e) {
//                Log.e("MSG", "IOException: " + e.getMessage());
            }

            Log.i("LST:", "Listenning");

            long startTime = System.currentTimeMillis();

            for(i = 0; i < 10; i++) {
                try{
                    DatagramPacket rcvPacket = new DatagramPacket(rcvBuffer, rcvBuffer.length);
                    socket.receive(rcvPacket);

                    String serverResponse = new String(rcvPacket.getData()).substring(0,17);

//                    Log.i("UDP_RESPONSE:", serverResponse);
                    result += serverResponse + "," + rcvPacket.getAddress().toString() + "," + String.valueOf(rcvPacket.getPort()) + ";";

                }catch (IOException e) {
//                    Log.e("RCV" + i, "IOException: " + e.getMessage());
                }

                if((System.currentTimeMillis() - startTime) >= SOCKET_TIMEOUT){
                    /* Enough time has passed for 30 devices to respond  */
                    break;
                }
            }

        } catch (SocketException e) {
//            Log.e("RCV", "SocketException: " + e.getMessage());
        }
        result += " ";

        /* All listed, now get info */
        String[] devices = result.split(";");

        result = "";

        for(i=0; i < devices.length; i++){
            String[] devInfoStr = devices[i].split(",");
            if(devInfoStr.length > 2){
                /* Some data exists */
                String IPAddr = devInfoStr[1];
                String idUrl = "http:/" + IPAddr + "/id";

                /* Now query id */
                String response = queryUrl(idUrl);

                if(response.length() < 2){
                    /* No data received. Try alternate URL. */

                    IPAddr += ":" + String.valueOf(BROADCAST_PORT + 1);
                    idUrl = "http:/" + IPAddr + "/id";

                    response = queryUrl(idUrl);
                }

                if(response.length() > 2){
                    result += "IP: " + IPAddr.replace("/", "") + "\n" + response + ";";
                }
            }
        }

        return result;
    }

    /* This should be called for device list to refresh after a delay */
    void scheduleRefresh(){
        btnRefresh.setEnabled(false);
        refreshHandler = new Handler();
        refreshHandler.postDelayed(refreshDelayer, 100);
    }

    /* Extracts MAC from device data string */
    private String getMac(String devData){
        String retVal = "";
        int i;
        String[] props = devData.split("\n");

        for(i = 0; i < props.length; i++){
            if(props[i].split(":")[0].equals("MAC")){
                retVal = props[i].substring(4, props[i].length());
                break;
            }
        }
//        Log.i("MAC CALC:", retVal);
        return retVal;
    }

    /* Extracts current value from device data string */
    private String getCurrent(String devData){
        String retVal = "";
        int i;
        String[] props = devData.split("\n");

        for(i = 0; i < props.length; i++){
            if(props[i].split(":")[0].equals("CURRENT")){
                retVal = props[i].split(":")[1];
                break;
            }
        }

        if(retVal.length() < 3){
            retVal += " ";
        }
        if(retVal.length() < 3){
            retVal += " ";
        }

        return retVal;
    }

    /* Refresh all devices on the same network. */
    private void refreshDevs(){
        btnRefresh.setEnabled(false);
        String scanResult = listDevices();
//        Log.i("DEVS", scanResult);

        deviceList.setAdapter(null);

        devices = scanResult.replace("{", "").replace("}", "").replace(",", "\n").replace("\"", "").split(";");

        if(devices[0].length() > 18) { /* 18 is just a tested size. It only shows there is at least size of a MAC address */

            /* Check if any labels are saved for detected devices and apply them */
            final String[] labels = new String[devices.length];
            int di;

            for(di = 0; di < devices.length; di++){
                String MAC = getMac(devices[di]);
                String label = readLabel(MAC);

                if(label.length() > 2){
                    labels[di] = label;
                }else{
                    labels[di] = devices[di];
                }
            }

            /* Populate the list with formed string array */
            deviceList.setAdapter(new ArrayAdapter(this, android.R.layout.simple_list_item_1, labels) {
                @Override
                public View getView(int position, View convertView, ViewGroup parent) {
                    View row = super.getView(position, convertView, parent);

                    if(position == lastClickedItemId){
                        // do something change color
                        row.setBackgroundColor(Color.parseColor("#F6E3CE"));
                    }
                    else{
                        // default state
                        row.setBackgroundColor (Color.WHITE); // default color
                    }
                    String currentLabel = String.valueOf(((TextView)row).getText());
                    if(!currentLabel.contains("CURRENT")){
                        String currentVal = getCurrent(devices[position]);

                        if(currentVal.length() > 10){
                            /* Too long, so display up to 30 characters */
                            currentLabel = "(" + currentVal.substring(0, Math.min(30, currentVal.length())) + ")\n" + currentLabel;
                        }else{
                            currentLabel = currentVal + ":" + currentLabel;
                        }

                        ((TextView)row).setText(currentLabel);
                    }

                    ((TextView) row).setTypeface(Typeface.MONOSPACE);
                    return row;
                }
            });

            /* If item is clicked, this should take us to the devices web UI */
            deviceList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    lastClickedItemId = position;

                    view.setBackgroundColor(Color.parseColor("#F6E3CE"));

                    String urlStr = "http://" + devices[position].split("\n")[0].split(": ")[1];

                    // Log.i("GoTo", urlStr);

                    Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(urlStr));
                    startActivity(browserIntent);
                }
            });

            /* On long click we are setting up the label */
            deviceList.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
                @Override
                public boolean onItemLongClick(AdapterView<?> adapterView, final View view, final int position, long l) {

                    final String devData = devices[position];

                    AlertDialog.Builder alertDialog = new AlertDialog.Builder(DeviceBrowser.this);
                    alertDialog.setTitle("Set Label");
                    alertDialog.setMessage( devData );

                    final EditText input = new EditText(DeviceBrowser.this);
                    ConstraintLayout.LayoutParams lp = new ConstraintLayout.LayoutParams(
                            ConstraintLayout.LayoutParams.MATCH_PARENT,
                            ConstraintLayout.LayoutParams.MATCH_PARENT);
                    input.setLayoutParams(lp);
                    if(!labels[position].equals(devData)){
                        input.setText(labels[position], TextView.BufferType.EDITABLE);
                    }

                    alertDialog.setView(input);

                    alertDialog.setPositiveButton("Apply",
                            new DialogInterface.OnClickListener(){
                                public void onClick (DialogInterface dialog,int which) {
                                    // Write your code here to execute after dialog
                                    String labelToSet = input.getText().toString();
                                    saveLabel(getMac(devData), labelToSet);
                                    labels[position] = labelToSet;


                                    if(labelToSet.length() < 3){
                                        labelToSet = devData;
                                    }else{
                                        String currentVal = getCurrent(devices[position]);

                                        if(currentVal.length() > 10){
                                            /* Too long, so display up to 30 characters */
                                            labelToSet = "(" + currentVal.substring(0, Math.min(30, currentVal.length())) + ")\n" + labelToSet;
                                        }else{
                                            labelToSet = currentVal + ":" + labelToSet;
                                        }
                                    }

                                    ((TextView)view).setText(labelToSet);
                                }
                            });

                    input.requestFocus();
                    alertDialog.show();

                    return true;
                }
            });
        }
        btnRefresh.setEnabled(true);
    }

    /* Turn all devices off. Sends a zero to control url of each device */
    private void allDevsOff(){
        int i, j;
        for(i=0; i<devices.length; i++){
//            Log.i("devs", devices[i]);
            String[] props = devices[i].split("\n");

            String urlStr = "";

            for(j = 0; j < props.length; j++){
                String[] prIdVal = props[j].split(":");

                if(prIdVal[0].equals("IP")){
                    urlStr = "http://" + prIdVal[1].replace(" ", "");
                }

                if(prIdVal[0].equals("CTRL_URL")){
                    urlStr += prIdVal[1] + "-2000";
                }
            }

            URL url;
            HttpURLConnection urlConnection = null;
            try {
                url = new URL(urlStr);

                urlConnection = (HttpURLConnection) url.openConnection();

                InputStream in = urlConnection.getInputStream();
                InputStreamReader isw = new InputStreamReader(in);

                String urlResp = "";
                int data = isw.read();
                while (data != -1) {
                    urlResp += (char) data;
                    data = isw.read();
                }
//                Log.i("RSP", urlResp);

            } catch (Exception e) {
//                Log.e("ERR", "Error getting dev_id: " + e.getMessage());
            } finally {
                if (urlConnection != null) {
                    urlConnection.disconnect();
                }
            }
        }

        refreshDevs(); /* To update current state */
    }

    private void saveLabel(String MAC, String label){
        SharedPreferences prefs = getSharedPreferences("srv_prefs", DeviceBrowser.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putString(MAC, label);

        editor.apply();
    }

    public String readLabel(String MAC) {
        SharedPreferences prefs = getSharedPreferences("srv_prefs", DeviceBrowser.MODE_PRIVATE);
        return prefs.getString(MAC, "");
    }

    private void CheckWiFiAvailable() {
        WifiManager manager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (manager.isWifiEnabled()) {
            WifiInfo wifiInfo = manager.getConnectionInfo();
            if (wifiInfo != null) {
                NetworkInfo.DetailedState state = WifiInfo.getDetailedStateOf(wifiInfo.getSupplicantState());
                if (state == NetworkInfo.DetailedState.CONNECTED || state == NetworkInfo.DetailedState.OBTAINING_IPADDR) {
                    statusText.setText("Connected to " + wifiInfo.getSSID());
                    if(wifiAvailableFlag == 2){
                        scheduleRefresh();
                    }

                    if(wifiAvailableFlag < 3) {
                        wifiAvailableFlag++;
                    }
                    return;
                }
            }
        }
        statusText.setText("No WiFi connection.");
        if(wifiAvailableFlag > 0){
            scheduleRefresh();
        }
        wifiAvailableFlag = 0;
    }

    Runnable WiFiStatusChecker = new Runnable() {
        @Override
        public void run() {
            try {
                CheckWiFiAvailable();
            } finally {
                wifiRefreshHandler.postDelayed(WiFiStatusChecker, 2000);
            }
        }
    };
}
