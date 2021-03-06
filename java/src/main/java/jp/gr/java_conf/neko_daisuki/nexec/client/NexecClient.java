package jp.gr.java_conf.neko_daisuki.nexec.client;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.nio.channels.Pipe;
import java.nio.channels.SocketChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.net.ssl.SSLContext;

import jp.gr.java_conf.neko_daisuki.fsyscall.io.SSLFrontEnd;
import jp.gr.java_conf.neko_daisuki.fsyscall.io.SyscallReadableChannel;
import jp.gr.java_conf.neko_daisuki.fsyscall.io.SyscallWritableChannel;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Application;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.FileMap;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Permissions;
import jp.gr.java_conf.neko_daisuki.fsyscall.slave.Slave;
import jp.gr.java_conf.neko_daisuki.fsyscall.util.VirtualPath;
import jp.gr.java_conf.neko_daisuki.fsyscall.util.SSLUtil;

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

    private static class ChannelPair {

        public SyscallReadableChannel in;
        public SyscallWritableChannel out;
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

    private Application mApplication;

    public int run(String server, int port, SSLContext context, String userName,
                   String password, String[] args,
                   VirtualPath currentDirectory, InputStream stdin,
                   OutputStream stdout, OutputStream stderr, Environment env,
                   Permissions permissions, FileMap fileMap,
                   Slave.Listener listener, String resourceDirectory)
                   throws ProtocolException, InterruptedException,
                          IOException {
        InetSocketAddress address = new InetSocketAddress(server, port);
        SocketChannel sock = SocketChannel.open(address);
        Pipe front2back = Pipe.open();
        Pipe back2front = Pipe.open();
        SSLFrontEnd sslFrontEnd = new SSLFrontEnd(context, sock,
                                                  back2front.source(),
                                                  front2back.sink());
        try {
            ChannelPair pair = new ChannelPair();
            pair.in = new SyscallReadableChannel(front2back.source());
            pair.out = new SyscallWritableChannel(back2front.sink());

            doLogin(pair, userName, password);
            sendEnvironment(pair, env);
            doExec(pair, args);

            sock.socket().setTcpNoDelay(true);
            mApplication = new Application();
            synchronized (this) {
                notifyAll();
            }
            return mApplication.run(
                    pair.in, pair.out,
                    currentDirectory,
                    stdin, stdout, stderr,
                    permissions, fileMap, listener, resourceDirectory);
        }
        finally {
            sslFrontEnd.join();
        }
    }

    public void cancel() {
        try {
            synchronized (this) {
                while (mApplication == null) {
                    wait();
                }
                mApplication.cancel();
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
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
        FileMap fileMap = new FileMap();
        try {
            SSLContext context = SSLContext.getInstance("TLS");
            context.init(null, null, null);

            client.run(
                    server, port, context, "anonymous", "anonymous", params,
                    new VirtualPath(args[2]), stdin, stdout, stderr, env,
                    perm, fileMap, null, "/tmp");
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    private void doSetEnv(ChannelPair pair, String name, String value) throws IOException, ProtocolException {
        Buffer buffer = new Buffer("SET_ENV");
        buffer.append(name);
        buffer.append(value);
        buffer.endOfLine();
        pair.out.write(buffer.getBytes());

        readOkOrDie(pair.in);
    }

    private void doLogin(ChannelPair pair, String userName,
                         String password) throws IOException,
                                                 ProtocolException {
        Buffer buffer = new Buffer("LOGIN");
        buffer.append(userName);
        buffer.append(password);
        buffer.endOfLine();
        pair.out.write(buffer.getBytes());

        readOkOrDie(pair.in);
    }

    private void doExec(ChannelPair pair, String[] args) throws IOException, ProtocolException {
        Buffer buffer = new Buffer("EXEC");
        for (String a: args) {
            buffer.append(a);
        }
        buffer.endOfLine();
        pair.out.write(buffer.getBytes());

        readOkOrDie(pair.in);
    }

    private String readLine(SyscallReadableChannel in) throws IOException, ProtocolException {
        List<Byte> buffer = new ArrayList<Byte>();
        byte c;
        while ((c = in.readByte()) != '\r') {
            buffer.add(Byte.valueOf(c));
        }
        if (in.readByte() != '\n') {
            throw new ProtocolException("invalid line terminator");
        }
        byte[] bytes = new byte[buffer.size()];
        for (int i = 0; i < buffer.size(); i++) {
            bytes[i] = buffer.get(i).byteValue();
        }
        return new String(bytes, ENCODING).trim();
    }

    private void readOkOrDie(SyscallReadableChannel in) throws IOException, ProtocolException {
        String line = readLine(in);
        if (!line.equals("OK")) {
            String fmt = "request was refused: %s";
            throw new ProtocolException(String.format(fmt, line));
        }
    }

    private void sendEnvironment(ChannelPair pair, Environment env) throws IOException, ProtocolException {
        for (String name: env.nameSet()) {
            doSetEnv(pair, name, env.get(name));
        }
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
