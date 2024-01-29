# What is this?

A demo, of working with protocol state-machine (reliability with retransmissions) over UDP using `libevent`.

## Building

Install libevent headers:
`
apt install libevent-dev
`

`
scons -j 4 build/demo
`

## Testing

`
scons -j 4 build/test && ./build/test
`
