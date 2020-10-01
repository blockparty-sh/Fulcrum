Our index is rocksdb backed with a key of block height pointing to a hat-trie of prefixnibbles pointing towards a set of TxNums

We point to TxNum's instead of TxHash's themselves to preserve memory and prevent memory explosion for worst case.

We store a vector, this could be further optimized with a small size optimized vector as there are around 1/65536 block txs inside each full 16 bit prefix, so there is generally only 1 tx per vector with current block sizes


HAT-trie provides very efficient and cache-friendly prefix searching as well as extremely fast serialization/deserialization.


height -> {
    trie{prefix => vec{idx}}
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

---

for each lookup we query the reusableblock then use the indexes of this during a full blockfetch to grab relevant txs
similarly to blockchain.transaction.get_id_from_pos


---

What is prefix format and why do we do prefix using weird hexstring format?

This is simplest format. Everything can use standard hex encoding/decoding.
(Maybe we wanna do just single digit though?)

The 4 bit size is enforced



**get_history**

height UINT block to start at
count UINT how many blocks to scan, max 2016
prefix HEXSTRING each hex digit must be less than 16 (4 bits each)
unspentonly BOOL optional, default FALSE
compact BOOL optional, default FALSE


**subscribe**
