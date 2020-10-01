Our index is rocksdb backed with a key of block height pointing to a hat-trie of prefixnibbles pointing towards TxNum

We point to TxNum's instead of TxHash's themselves to preserve memory and prevent memory explosion for worst case.

This provides very efficient and cache-friendly prefix searching as well as extremely fast serialization/deserialization.


height -> {
    vec{idx => txid}
    trie{prefix => idx}
}

----

we use a hat-trie with each symbol being a 4 bit value
tsl/hat-trie supports fast prefix searching as well as serialization/deserialization
configure the burst parameter (and others?) for txids


using PrefixNibble = char; // only first 4 bits used
using TxHashIdx = size_t; // index of txHashes (should this be 4bytes for memory/storage consideration??)

ReusableBlock
{
    std::vector<TxHash> txHashes;
    htrie_map<PrefixNibble, TxNum>  
};

we might be able to use the Storage system rather thna storing all txids inside these reusableblocks
