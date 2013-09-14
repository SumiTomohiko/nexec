package jp.gr.java_conf.neko_daisuki.nexec.client;

import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Application;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Links;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Permissions;

public class NexecClient {

    public static class Environment {

        private Map<String, String> mMap;

        public Environment() {
            mMap = new HashMap<String, String>();
        }

        public void put(String name, String value) {
            mMap.put(name, value);
        }

        public Set<String> nameSet() {
            return mMap.keySet();
        }

        public String get(String name) {
            return mMap.get(name);
        }
    }

    private static class StreamPair {

        public InputStream in;
        public OutputStream out;
    }

    private static class Buffer {

        private StringBuilder mBuffer;

        public Buffer(String cmd) {
            mBuffer = new StringBuilder(cmd);
        }

        public void append(String token) {
            mBuffer.append(String.format(" %s", token));
        }

        public void endOfLine() {
            append("\r\n");
        }

        public byte[] getBytes() throws UnsupportedEncodingException {
            return mBuffer.toString().getBytes(ENCODING);
        }
    }

    private static final String ENCODING = "UTF-8";

    public void run(String server, int port, String[] args, InputStream stdin, OutputStream stdout, OutputStream stderr, Environment env, Permissions permissions, Links links) throws ProtocolException, InterruptedException, IOException {
        Socket sock = new Socket(server, port);
        try {
            StreamPair pair = new StreamPair();
            pair.in = sock.getInputStream();
            pair.out = sock.getOutputStream();

            sendEnvironment(pair, env);
            doExec(pair, args);

            sock.setTcpNoDelay(true);
            new Application().run(
                    pair.in, pair.out,
                    stdin, stdout, stderr,
                    permissions, links);
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
        InputStream stdin = System.in;
        OutputStream stdout = System.out;
        OutputStream stderr = System.err;
        Environment env = new Environment();
        Permissions perm = new Permissions(true);
        Links links = new Links();
        try {
            client.run(
                    server, port, params,
                    stdin, stdout, stderr,
                    env, perm, links);
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    private void doSetEnv(StreamPair pair, String name, String value) throws IOException, ProtocolException {
        Buffer buffer = new Buffer("SET_ENV");
        buffer.append(name);
        buffer.append(value);
        buffer.endOfLine();
        pair.out.write(buffer.getBytes());

        readOkOrDie(pair.in);
    }

    private void doExec(StreamPair pair, String[] args) throws IOException, ProtocolException {
        Buffer buffer = new Buffer("EXEC");
        for (String a: args) {
            buffer.append(a);
        }
        buffer.endOfLine();
        pair.out.write(buffer.getBytes());

        readOkOrDie(pair.in);
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

    private void sendEnvironment(StreamPair pair, Environment env) throws IOException, ProtocolException {
        for (String name: env.nameSet()) {
            doSetEnv(pair, name, env.get(name));
        }
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
