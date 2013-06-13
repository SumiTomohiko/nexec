package jp.gr.java_conf.neko_daisuki.nexec.client;

import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.util.Arrays;

import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Application;

public class NexecClient {

    public void run(String server, int port, String[] args) throws InterruptedException, IOException {
        Socket sock = new Socket(server, port);
        try {
            OutputStream out = sock.getOutputStream();
            DataOutput dataOut = new DataOutputStream(out);
            dataOut.writeInt(args.length);
            for (String s: args) {
                byte[] bytes = s.getBytes("UTF-8");
                dataOut.writeInt(bytes.length);
                dataOut.write(bytes);
            }

            new Application().run(sock.getInputStream(), out);
        }
        finally {
            sock.close();
        }
    }

    public static void main(String[] args) {
        String server = args[0];
        int port = Integer.parseInt(args[1]);
        String[] params = Arrays.copyOfRange(args, 2, args.length);

        try {
            new NexecClient().run(server, port, params);
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
