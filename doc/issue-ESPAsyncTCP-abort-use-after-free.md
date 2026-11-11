# ESP8266: `AsyncWebServerRequest::abort()` corrupts the heap via `AsyncClient::abort()` (ESPAsyncTCP) – crashes on malformed requests / port scans

> **Why here:** the root cause is in `ESPAsyncTCP` (`AsyncClient::abort()`), but
> [ESP32Async/ESPAsyncTCP](https://github.com/ESP32Async/ESPAsyncTCP) is archived (read-only), so I can't open an issue there.
> ESPAsyncWebServer is the main user of this code path and lists `ESP32Async/ESPAsyncTCP @ 2.0.0` as its ESP8266 dependency.
> Only ESP8266 is affected; ESP32 uses AsyncTCP.

## Summary

On ESP8266, ESPAsyncWebServer crashes when it receives a malformed request, for example a NUL byte in the
request line or headers, or a TLS ClientHello on port 80. Network scanners send exactly this kind of data, for example
`nmap -sV` style service detection or network inventory tools. The device then crashes
within seconds to minutes, with changing exceptions at unrelated places.

The cause is two bugs in `AsyncClient::abort()` (ESPAsyncTCP 2.0.0, same code on the last `main`):

1. **Use-after-free of the client.** `tcp_abort()` calls the error callback synchronously, and ESPAsyncWebServer
   deletes the request **and the `AsyncClient`** there. After `tcp_abort()` returns, `abort()` still writes
   `_pcb = NULL` into the freed object and calls `setCloseError()` on it.
2. **Wrong return value to lwIP.** When `abort()` is called inside a lwIP callback of the same connection,
   as `AsyncWebServerRequest::_onData()` does, the callback returns `ERR_OK` instead of `ERR_ABRT`. lwIP then
   keeps using the pcb that `tcp_abort()` has already freed.

Both end in heap corruption.

## Environment

- ESPAsyncWebServer 3.10.0
- ESPAsyncTCP 2.0.0 (also checked: the last `main` of ESP32Async/ESPAsyncTCP has the same code)
- ESP8266 Arduino core 3.1.2 (also seen with 3.2.0-dev git master), PlatformIO, Wemos D1 mini
- NONOS SDK 2.2.x and 3.0.5: both affected

## Where ESPAsyncWebServer triggers it

`AsyncWebServerRequest::_onData()` runs in the lwIP **recv callback** and calls `abort()` on invalid data:

```cpp
// WebRequest.cpp, AsyncWebServerRequest::_onData()
if (_parseState == PARSE_REQ_START && len && ((uint8_t *)buf)[0] == 0x16) {  // TLS handshake
  _parseState = PARSE_REQ_FAIL;
  abort();
  return;
}
...
if (!str[i]) {               // NUL character in header
  _parseState = PARSE_REQ_FAIL;
  abort();
  return;
}
```

`AsyncWebServerRequest::abort()` → `_client->abort()` → `tcp_abort(_pcb)`, and then:

1. lwIP calls the err callback synchronously → `AsyncClient::_s_error()` → `_error()`
2. → `_discard_cb` → `AsyncWebServerRequest::_onDisconnect()` → `AsyncWebServer::_handleDisconnect()`
   → `delete request` → `~AsyncWebServerRequest()` → **`delete _client`**
3. Back in `AsyncClient::abort()`: `_pcb = NULL;` writes into the freed block, and `setCloseError()`
   dereferences `_errorTracker` of the freed object (**bug 1**).
4. Back in lwIP (`tcp_input()`): the recv callback returned `ERR_OK`, so lwIP continues to use the pcb
   that `tcp_abort()` has freed (**bug 2**).

The same applies to the accept callback in `AsyncWebServer`'s constructor (`c->abort(); delete c;` when
`new AsyncWebServerRequest` fails), and to any user code that rejects clients in `onClient()`.

### Why it corrupts the heap

- `ESPAsyncTCP` code:

  ```cpp
  void AsyncClient::abort(){
    ...
    if(_pcb) {
      tcp_abort(_pcb);
      _pcb = NULL;              // <-- write to possibly deleted object
      setCloseError(ERR_ABRT);  // <-- read of possibly deleted object (_errorTracker)
    }
    return;
  }
  ```

- `_pcb` is the **first member** of `AsyncClient`, and the class has no virtual functions, so it sits at
  offset 0. In a freed umm_malloc block, the first bytes hold the free-list links. Writing `NULL` there
  corrupts the heap free list.
- `tcp_abort()` → `tcp_abandon()` frees the pcb (`tcp_free(pcb)`) and **then** calls the error callback with
  `ERR_ABRT`. `AsyncClient::_s_error()` marks the tracker as errored (`setErrored(EE_ERROR_CB)`), so
  `ACErrorTracker::getCallbackCloseError()` returns **`ERR_OK`** instead of `ERR_ABRT`. The lwIP rule is
  *"Only return ERR_ABRT if you have called tcp_abort from within the callback function"*. In lwIP 2.1.3
  (`tcp_in.c`), the recv path continues to use `pcb` unless the callback returns `ERR_ABRT`, and the accept
  path (`tcp_process()`, state `SYN_RCVD`) continues with `tcp_receive(pcb)` on `ERR_OK`.
- On ESP8266, lwIP pcbs are heap allocated (`MEMP_MEM_MALLOC 1`), so this is heap corruption as well.

The result is random crashes later, at unrelated locations, which makes the root cause very hard to find.

## How to reproduce

Run any ESPAsyncWebServer on an ESP8266 and send a few TCP connections with binary payloads to port 80:

```python
import socket, time
HOST = "<device-ip>"
probes = [b"\x00\x00\x07version\x04bind\x00\x00\x10\x00\x03",   # DNS version.bind
          b"\x00\x05AAAA",
          b"\x00\x08\x00\x0b\x00\x00",
          b"\x00postgres\x00database\x00postgres\x00",
          b"\x00\x00\x00\x02\x00\x01\x86\xa0\x00\x01\x97|\x00\x00\x00\x00"]
for r in range(10):
    for p in probes:
        try:
            s = socket.socket(); s.settimeout(2); s.connect((HOST, 80)); s.sendall(p)
            try: s.recv(100)
            except Exception: pass
            s.close()
        except Exception:
            pass
        time.sleep(0.07)
    time.sleep(1)
```

The device crashes within seconds to minutes, reproducibly, with changing exceptions, for example:

- `Exception (28)` / `Exception (29)` in `pbuf_free()` (called from `ip4_input()`)
- `Exception (29)` at `0x4000df64` (`memcpy` to NULL) from `ieee80211_setup_ratetable()`
- `Abort/Panic`, `std::bad_alloc` in otherwise harmless allocations

The free heap stayed above 7 KB in every case, up to 24 KB, so this was not memory exhaustion.
In my setup, a network inventory tool on the LAN (deep scan with service probes) crashed the device every
few minutes. After excluding the device from that scan it ran stably, and after re-enabling the scan it
crashed within minutes.

## Proposed fix (in ESPAsyncTCP)

Fix both bugs in `AsyncClient::abort()`:

1. Clear the members and keep the error tracker alive **before** calling `tcp_abort()`, and do not touch
   `this` afterwards.
2. After `tcp_abort()`, reset the tracker so that the enclosing callback returns `ERR_ABRT` to lwIP exactly
   once. `getCallbackCloseError()` then sets `EE_ABORTED`. `AsyncClient` is a friend of `ACErrorTracker`,
   and the local `shared_ptr` keeps the tracker alive.

Diff against the last `main` of ESP32Async/ESPAsyncTCP (`src/ESPAsyncTCP.cpp`):

```diff
--- a/src/ESPAsyncTCP.cpp
+++ b/src/ESPAsyncTCP.cpp
@@ -344,10 +344,19 @@
   //    of a 2nd call to tcp_abort().
   // 6) Callbacks to _recv() or _connected() with err set, will result in _pcb
   //    set to NULL. Thus, preventing possible calls later to tcp_abort().
+  // 7) tcp_abort() calls the error callback synchronously - the owner may delete
+  //    this object there. So clear the members first and do not touch them afterwards.
+  // 8) tcp_abort() has freed the pcb - if we are inside a lwIP callback of this
+  //    connection, it must return ERR_ABRT, otherwise lwIP uses the freed pcb.
+  //    _s_error() marked the tracker as errored (=> ERR_OK) - reset it.
   if(_pcb) {
-    tcp_abort(_pcb);
+    tcp_pcb *pcb = _pcb;
     _pcb = NULL;
-    setCloseError(ERR_ABRT);
+    std::shared_ptr<ACErrorTracker> errorTracker = _errorTracker;   // keeps the tracker alive
+    errorTracker->setCloseError(ERR_ABRT);
+    tcp_abort(pcb);       // may delete this !
+    errorTracker->_errored     = EE_OK;       // => getCallbackCloseError() returns ERR_ABRT once
+    errorTracker->_close_error = ERR_ABRT;
   }
   return;
 }
```

Notes:

- `_pcb` is already `NULL` when `_error()` runs. `_error()` checks `if(_pcb)`, so it skips the SSL free /
  reset, which is fine because the pcb is being aborted. The error and discard callbacks are still called as
  before, so owners are notified and can delete the client.
- If `abort()` is called outside of a lwIP callback, nobody reads the tracker afterwards. The pcb is gone,
  so no further callbacks arrive.
- A cleaner way might be a dedicated flag in `ACErrorTracker`, such as "aborted by the application".
  I kept the patch minimal.

## Test results

ESP8266 (D1 mini), ESPAsyncWebServer 3.10.0, using the reproduction script above in a loop:

| Firmware | Result |
| -- | -- |
| unpatched | Exception (28)/(29) in the first round, reproducible |
| fix 1 only | about 5 minutes of probes, then a hang / hardware watchdog reset (battery powered) |
| fix 1 + fix 2, USB powered | **30 minutes, 1045 rounds (5225 probe connections), no reset**, free heap stable at 17–20 KB |
| fix 1 + fix 2, battery powered | **2 × 30 minutes, 2205 rounds (11025 probe connections), no reset**, free heap 9–23 KB |

One earlier battery-powered run with fix 1 + fix 2 ended in a hardware watchdog reset after about 21 minutes,
without an exception dump. It could not be reproduced in the runs listed above. A supply dip is possible
(LED load on the same battery), so I don't count it as a software result.

## Workaround for users

Until a fix is released, the patch can be applied automatically before every build with a PlatformIO
pre-script:
[`scripts/patchLibs.py`](https://github.com/LSC-Labs/PLibESPV1/blob/main/scripts/patchLibs.py)
(LSC-Labs/PLibESPV1). Copy it into your project and add it to `platformio.ini`:

```ini
extra_scripts = pre:scripts/patchLibs.py
```

The script replaces `abort()` in `.pio/libdeps/<env>/ESPAsyncTCP/src/ESPAsyncTCP.cpp`. It is idempotent,
and it prints a warning if the original code is no longer found, for example after a fixed release.

## Question to the maintainers

Since ESPAsyncTCP is archived, how would you like to handle this?

1. Unarchive ESP32Async/ESPAsyncTCP and release a 2.0.1 with the fix above. This is the cleanest option,
   because it also fixes other users of `AsyncClient::abort()`, such as AsyncMqttClient and user code in `onClient()`.
2. Work around it in ESPAsyncWebServer for ESP8266, for example by not calling `_client->abort()`
   synchronously from `_onData()` / the accept callback, but deferring it or using `close()` instead.
3. Document the issue and the workaround, for example in the README / ESP8266 notes.

I'm happy to provide a PR for option 1 or 2.
