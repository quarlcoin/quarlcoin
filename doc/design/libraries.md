# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libquarlcoin_cli*         | RPC client functionality used by *quarl-cli* executable |
| *libquarlcoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libquarlcoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libquarlcoin_consensus*   | Consensus functionality used by *libquarlcoin_node* and *libquarlcoin_wallet*. |
| *libquarlcoin_crypto*      | Hardware-optimized functions for data encryption, hashing, message authentication, and key derivation. |
| *libquarlcoin_kernel*      | Consensus engine and support library used for validation by *libquarlcoin_node*. |
| *libquarlcoinqt*           | GUI functionality used by *quarl-qt* and *quarlcoin-gui* executables. |
| *libquarlcoin_ipc*         | IPC functionality used by *quarl-node* and *quarlcoin-gui* executables to communicate when [`-DENABLE_IPC=ON`](multiprocess.md) is used. |
| *libquarlcoin_node*        | P2P and RPC server functionality used by *quarld* and *quarl-qt* executables. |
| *libquarlcoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libquarlcoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libquarlcoin_wallet*      | Wallet functionality used by *quarld* and *quarl-wallet* executables. |
| *libquarlcoin_wallet_tool* | Lower-level wallet functionality used by *quarl-wallet* executable. |
| *libquarlcoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *quarld* and *quarl-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. An exception is *libquarlcoin_kernel*, which, at some future point, will have a documented external interface.

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`add_library(quarlcoin_* ...)`](../../src/CMakeLists.txt) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libquarlcoin_node* code lives in `src/node/` in the `node::` namespace
  - *libquarlcoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libquarlcoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libquarlcoin_util* code lives in `src/util/` in the `util::` namespace
  - *libquarlcoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "basis" }}}%%

graph TD;

quarl-cli[quarl-cli]-->libquarlcoin_cli;

quarld[quarld]-->libquarlcoin_node;
quarld[quarld]-->libquarlcoin_wallet;

quarl-qt[quarl-qt]-->libquarlcoin_node;
quarl-qt[quarl-qt]-->libquarlcoinqt;
quarl-qt[quarl-qt]-->libquarlcoin_wallet;

quarl-wallet[quarl-wallet]-->libquarlcoin_wallet;
quarl-wallet[quarl-wallet]-->libquarlcoin_wallet_tool;

libquarlcoin_cli-->libquarlcoin_util;
libquarlcoin_cli-->libquarlcoin_common;

libquarlcoin_consensus-->libquarlcoin_crypto;

libquarlcoin_common-->libquarlcoin_consensus;
libquarlcoin_common-->libquarlcoin_crypto;
libquarlcoin_common-->libquarlcoin_util;

libquarlcoin_kernel-->libquarlcoin_consensus;
libquarlcoin_kernel-->libquarlcoin_crypto;
libquarlcoin_kernel-->libquarlcoin_util;

libquarlcoin_node-->libquarlcoin_consensus;
libquarlcoin_node-->libquarlcoin_crypto;
libquarlcoin_node-->libquarlcoin_kernel;
libquarlcoin_node-->libquarlcoin_common;
libquarlcoin_node-->libquarlcoin_util;

libquarlcoinqt-->libquarlcoin_common;
libquarlcoinqt-->libquarlcoin_util;

libquarlcoin_util-->libquarlcoin_crypto;

libquarlcoin_wallet-->libquarlcoin_common;
libquarlcoin_wallet-->libquarlcoin_crypto;
libquarlcoin_wallet-->libquarlcoin_util;

libquarlcoin_wallet_tool-->libquarlcoin_wallet;
libquarlcoin_wallet_tool-->libquarlcoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class quarl-qt,quarld,quarl-cli,quarl-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Crypto* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus, crypto, and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libquarlcoin_wallet* and *libquarlcoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libquarlcoin_crypto* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libquarlcoin_consensus* should only depend on *libquarlcoin_crypto*, and all other libraries besides *libquarlcoin_crypto* should be allowed to depend on it.

- *libquarlcoin_util* should be a standalone dependency that any library can depend on, and it should not depend on other libraries except *libquarlcoin_crypto*. It provides basic utilities that fill in gaps in the C++ standard library and provide lightweight abstractions over platform-specific features. Since the util library is distributed with the kernel and is usable by kernel applications, it shouldn't contain functions that external code shouldn't call, like higher level code targeted at the node or wallet. (*libquarlcoin_common* is a better place for higher level code, or code that is meant to be used by internal applications only.)

- *libquarlcoin_common* is a home for miscellaneous shared code used by different Quarlcoin applications. It should not depend on anything other than *libquarlcoin_util*, *libquarlcoin_consensus*, and *libquarlcoin_crypto*.

- *libquarlcoin_kernel* should only depend on *libquarlcoin_util*, *libquarlcoin_consensus*, and *libquarlcoin_crypto*.

- The only thing that should depend on *libquarlcoin_kernel* internally should be *libquarlcoin_node*. GUI and wallet libraries *libquarlcoinqt* and *libquarlcoin_wallet* in particular should not depend on *libquarlcoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be able to get it from *libquarlcoin_consensus*, *libquarlcoin_common*, *libquarlcoin_crypto*, and *libquarlcoin_util*, instead of *libquarlcoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libquarlcoinqt*, *libquarlcoin_node*, *libquarlcoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](../../src/interfaces/) abstract interfaces.

## Work in progress

- Validation code is moving from *libquarlcoin_node* to *libquarlcoin_kernel* as part of [The libquarlcoinkernel Project #27587](https://github.com/quarlcoin/quarlcoin/issues/27587)
