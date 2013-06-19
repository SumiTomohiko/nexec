package jp.gr.java_conf.neko_daisuki.nexec.client;

import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Application;

public class NexecClient {

    private static class ProtocolException extends Exception {

        public ProtocolException(String message) {
            super(message);
        }
    }

    private static final String ENCODING = "UTF-8";

    public void run(String server, int port, String[] args, InputStream stdin, OutputStream stdout, OutputStream stderr) throws ProtocolException, InterruptedException, IOException {
        Socket sock = new Socket(server, port);
        try {
            InputStream in = sock.getInputStream();
            OutputStream out = sock.getOutputStream();
            doExec(in, out, args);

            new Application().run(in, out, stdin, stdout, stderr);
        }
        finally {
            sock.close();
        }
    }

    public static void main(String[] args) {
        String server = args[0];
        int port = Integer.parseInt(args[1]);
        String[] params = Arrays.copyOfRange(args, 2, args.length);

        NexecClient client = new NexecClient();
        try {
            client.run(server, port, params, System.in, System.out, System.err);
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    private void doExec(InputStream in, OutputStream out, String[] args) throws IOException, ProtocolException {
        StringBuilder buffer = new StringBuilder("EXEC");
        for (String a: args) {
            buffer.append(String.format(" %s", a));
        }
        buffer.append("\r\n");
        out.write(buffer.toString().getBytes(ENCODING));

        readOkOrDie(in);
    }

    private String readLine(InputStream in) throws IOException, ProtocolException {
        List<Byte> buffer = new ArrayList<Byte>();
        int c;
        while ((c = in.read()) != '\r') {
            buffer.add(Byte.valueOf((byte)c));
        }
        if (in.read() != '\n') {
            throw new ProtocolException("invalid line terminator");
        }
        byte[] bytes = new byte[buffer.size()];
        for (int i = 0; i < buffer.size(); i++) {
            bytes[i] = buffer.get(i).byteValue();
        }
        return new String(bytes, ENCODING).trim();
    }

    private void readOkOrDie(InputStream in) throws IOException, ProtocolException {
        String line = readLine(in);
        if (!line.equals("OK")) {
            String fmt = "request was refused: %s";
            throw new ProtocolException(String.format(fmt, line));
        }
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
