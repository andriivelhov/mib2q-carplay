import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.app.decompiler.*;
import java.util.*;

/* Recover only the client control boundaries, including dispatched callbacks. */
public class DumpNativeProxy extends GhidraScript {
  public void run() throws Exception {
    Set<Function> targets = new LinkedHashSet<>();
    String[] names = {"AirplayClientSendCmdSetupScreen", "AirplayClientSendCmdTearDownScreen",
      "_ProcessProxyRequestCommand", "AirplayClientSendCmdCmd", "AirplayClientSendCmdSetup",
      "TeardownProxyClient", "_TeardownProxyClient", "AirplayClientSendCmdFeedback", "ProxyPairingSessionDeriveKey"};
    FunctionManager fm = currentProgram.getFunctionManager();
    for (Function f : fm.getFunctions(true)) {
      for (String n : names) if (f.getName().equals(n)) targets.add(f);
    }
    for (Function f : fm.getFunctions(true)) {
      if (f.getName().equals("_ProcessProxyRequestCommand") ||
          f.getName().equals("AirPlayReceiverSessionSetDelegate") ||
          f.getName().equals("CreateHTTPRequestMsg") ||
          f.getName().equals("CFBinaryPlistV0CreateWithData")) {
        for (Reference r : currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint())) {
          Function caller = fm.getFunctionContaining(r.getFromAddress());
          if (caller != null) targets.add(caller);
        }
      }
    }
    DecompInterface dc = new DecompInterface(); dc.openProgram(currentProgram);
    // Include local functions referenced by each exported dispatcher/callback.
    for (Function f : new ArrayList<>(targets)) {
      InstructionIterator ins = currentProgram.getListing().getInstructions(f.getBody(), true);
      while (ins.hasNext()) for (Reference r : ins.next().getReferencesFrom()) {
        Function callee = fm.getFunctionAt(r.getToAddress());
        if (callee != null && callee.getName().startsWith("FUN_")) targets.add(callee);
      }
    }
    for (String address : getScriptArgs()) {
      Function f = fm.getFunctionAt(toAddr(address));
      if(f != null) targets.add(f);
    }
    for (Function f : targets) {
      println("\n===== " + f.getName() + " @ " + f.getEntryPoint() + " =====");
      DecompileResults result = dc.decompileFunction(f, 120, monitor);
      if(result.getDecompiledFunction() != null) println(result.getDecompiledFunction().getC());
    }
    dc.dispose();
  }
}
