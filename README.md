<h1 align="center">
<br>
<br>
PhaseDive (Ekko fork)
</h1>


This is a PoC for a change to Ekko to use trampoline calls to ZwContinue and a `jmp rax` gadget to call functions from the `CONTEXT` struct. The `ntdll.dll` gadget is static, you need to find your own `call <ntdll.ZwContinue>` to test this<br>

### Credit
- Ekko implementation by C5pider (original repository)
- [Austin Hudson (@SecIdiot)](https://twitter.com/ilove2pwn_) https://suspicious.actor/2022/05/05/mdsec-nighthawk-study.html
- Originally discovered by [Peter Winter-Smith](peterwintrsmith) and used in MDSec’s Nighthawk
