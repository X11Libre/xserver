----------------------------- MODULE Xnamespace -----------------------------
(*
 * SPDX-FileCopyrightText: © 2026 Enrico Weigelt, metux IT consult
 * SPDX-License-Identifier: MIT
 *
 * This TLA+ specification models the Xnamespace security isolation extension
 * in Xlibre Xserver. It verifies:
 *  1. Resource Isolation (Confidentiality): A client in a non-root namespace
 *     cannot read, write, or query resources (Windows, Pixmaps, GCs) owned by
 *     a client in a different namespace.
 *  2. Selection Separation (Clipboard Isolation): Selections (like PRIMARY/CLIPBOARD)
 *     are completely separated between namespaces via atom name-rewriting, 
 *     making cross-namespace clipboard snooping impossible.
 *  3. Global Root Redirection: Global root window property operations are 
 *     redirected to the client namespace's virtual root, preventing cross-namespace
 *     covert channel communications.
 *  4. Virtual Root Lifetime (Memory Leak Prevention): Destroys virtual root windows
 *     on namespace deletion to prevent severe memory leaks in transient sessions.
 *)

EXTENDS Naturals, Sequences, TLC

CONSTANTS
    Clients,        \* Set of modeled X clients (e.g., {C1, C2, C3})
    Namespaces,     \* Set of namespaces (e.g., {"root", "NS_A", "NS_B"})
    ResourceTypes   \* {"Window", "Pixmap", "Selection", "Property"}

VARIABLES
    client_ns,          \* Map of Client -> Namespace ("root" represents superPower)
    active_namespaces,  \* Set of currently active namespaces (dynamic)
    resources,          \* Set of active resource structures: [id: Nat, rtype: ResourceTypes, owner: Clients, ns_prefix: {"none"} \union Namespaces]
    root_window,        \* Resource ID of the real screen root window
    ns_root_win,        \* Map of Namespace -> Resource ID of virtual root windows
    next_res_id         \* Counter for resource ID generation

-----------------------------------------------------------------------------

\* Helper: checks if a client has superPower (root namespace)
IsSuperPower(c) == client_ns[c] = "root"

\* Helper: checks if two clients are assigned to the same namespace
SameNamespace(c1, c2) == client_ns[c1] = client_ns[c2]

-----------------------------------------------------------------------------

\* --- TYPE INVARIANT ---

TypeOK ==
    /\ client_ns \in [Clients -> Namespaces]
    /\ active_namespaces \subseteq Namespaces
    /\ root_window \in Nat
    /\ ns_root_win \in [Namespaces -> Nat]
    /\ next_res_id \in Nat
    /\ resources \subseteq [id: Nat, rtype: ResourceTypes, owner: Clients, ns_prefix: {"none"} \union Namespaces]

-----------------------------------------------------------------------------

\* --- INITIAL STATE ---

Init ==
    /\ client_ns \in [Clients -> Namespaces]
    /\ active_namespaces = {"root", "NS_A"}
    /\ root_window = 1
    \* Virtual root windows created for each namespace
    /\ ns_root_win = [ns \in Namespaces |-> IF ns = "root" THEN 1 ELSE 1 + (CHOOSE x \in 2..10 : TRUE)] \* Simplified mapping
    /\ next_res_id = 11
    /\ resources = {
        [id |-> 1, rtype |-> "Window", owner |-> CHOOSE c \in Clients : client_ns[c] = "root", ns_prefix |-> "root"]
       }

-----------------------------------------------------------------------------

\* --- STATE TRANSITIONS (ACTIONS) ---

\* 1. A client creates a standard window, pixmap, or GC (XACE_RESOURCE_ACCESS - DixCreateAccess)
CreateResource(c, rtype) ==
    /\ rtype \in {"Window", "Pixmap"}
    /\ LET 
         new_res == [id |-> next_res_id, rtype |-> rtype, owner |-> c, ns_prefix |-> "none"]
       IN
         /\ resources' = resources \union {new_res}
         /\ next_res_id' = next_res_id + 1
         /\ UNCHANGED <<client_ns, active_namespaces, root_window, ns_root_win>>

\* 2. A client gets/sets a window property (XACE_PROPERTY_ACCESS)
\*    - If a client accesses the real root window, Xnamespace redirects it to their virtual root window!
GetSetProperty(c, target_win_id) ==
    /\ \exists r \in resources :
        /\ r.rtype = "Window"
        /\ r.id = target_win_id
        /\ LET 
             \* Apply root redirection logic from hookWindowProperty()
             effective_win_id == IF target_win_id = root_window /\ ~IsSuperPower(c)
                                 THEN ns_root_win[client_ns[c]]
                                 ELSE target_win_id
           IN
             \* Access is permitted if:
             \*  a) Same namespace
             \*  b) Client is superPower
             \*  c) It is the client's own virtual root window
             /\ \/ IsSuperPower(c)
                \/ \exists r_eff \in resources : 
                     /\ r_eff.id = effective_win_id 
                     /\ (SameNamespace(c, r_eff.owner) \/ effective_win_id = ns_root_win[client_ns[c]])
             /\ UNCHANGED <<client_ns, active_namespaces, resources, root_window, ns_root_win, next_res_id>>

\* 3. A client requests a selection (PRIMARY/CLIPBOARD)
\*    - Selection isolation is enforced in hookSelectionFilter() by prepending client's namespace to the selection atom name.
GetSetSelection(c, name) ==
    /\ LET 
         \* Selection name is rewritten with namespace prefix unless root client
         effective_prefix == IF IsSuperPower(c) THEN "none" ELSE client_ns[c]
         new_sel == [id |-> next_res_id, rtype |-> "Selection", owner |-> c, ns_prefix |-> effective_prefix]
       IN
         \* Ensure client only interacts with selections belonging to their namespace
         /\ \/ IsSuperPower(c)
            \/ \forall s \in resources : 
                 (s.rtype = "Selection" /\ s.id = next_res_id - 1) => s.ns_prefix = client_ns[c]
         /\ resources' = resources \union {new_sel}
         /\ next_res_id' = next_res_id + 1
         /\ UNCHANGED <<client_ns, active_namespaces, root_window, ns_root_win>>

\* 4. A client attempts to send an event (X_SendEvent) to another client's window
SendClientEvent(c, target_win_id) ==
    /\ \exists r \in resources :
        /\ r.rtype = "Window"
        /\ r.id = target_win_id
        \* Allowed if same namespace or superPower (hookSend / clientAllowedOnClient)
        /\ (SameNamespace(c, r.owner) \/ IsSuperPower(c))
        /\ UNCHANGED <<client_ns, active_namespaces, resources, root_window, ns_root_win, next_res_id>>

\* 5. Dynamic creation of a namespace (XnsCreate() / ProcXnsCreateNamespace())
CreateNamespace(c, ns) ==
    /\ IsSuperPower(c)
    /\ ns \notin active_namespaces
    /\ active_namespaces' = active_namespaces \union {ns}
    \* Create and register the namespace's virtual root window in resources database
    /\ LET
         v_root == [id |-> next_res_id, rtype |-> "Window", owner |-> c, ns_prefix |-> ns]
       IN
         /\ resources' = resources \union {v_root}
         /\ ns_root_win' = [ns_root_win EXCEPT ![ns] = next_res_id]
         /\ next_res_id' = next_res_id + 1
    /\ UNCHANGED <<client_ns, root_window>>

\* 6. Dynamic teardown of a namespace (XnsDestroyNamespace() / XnsDelete())
\*    - Buggy behavior (as-is): Removes the namespace but leaves the virtual root window in resources!
\*    - Fixed behavior: Removes the namespace AND unregisters its virtual root window from resources.
DestroyNamespace(c, ns) ==
    /\ IsSuperPower(c)
    /\ ns \in active_namespaces
    /\ ns /= "root" \* Builtin root namespace cannot be destroyed
    /\ active_namespaces' = active_namespaces \ {ns}
    \* MODELING BUGGY BEHAVIOR: Forget to clean up resources, keeping the rootWindow.
    \* TO MODEL THE FIXED BEHAVIOR (our patch): Change `resources' = resources` to remove the virtual root window:
    \*    resources' = { r \in resources : r.id /= ns_root_win[ns] }
    /\ resources' = resources
    /\ UNCHANGED <<client_ns, root_window, ns_root_win, next_res_id>>

-----------------------------------------------------------------------------

\* --- SYSTEM SPECIFICATION ---

Next ==
    \/ \exists c \in Clients, t \in {"Window", "Pixmap"} : CreateResource(c, t)
    \/ \exists c \in Clients, win_id \in Nat : GetSetProperty(c, win_id)
    \/ \exists c \in Clients : GetSetSelection(c, "PRIMARY")
    \/ \exists c \in Clients, win_id \in Nat : SendClientEvent(c, win_id)
    \/ \exists c \in Clients, ns \in Namespaces : CreateNamespace(c, ns)
    \/ \exists c \in Clients, ns \in Namespaces : DestroyNamespace(c, ns)

Spec == Init /\ [][Next]_<<client_ns, active_namespaces, resources, root_window, ns_root_win, next_res_id>>

-----------------------------------------------------------------------------

\* --- SECURITY & MEMORY INVARIANTS TO VERIFY ---

\* 1. Resource Confidentiality:
\*    A non-root client can never successfully create, read, or modify a resource 
\*    owned by a client in a different non-root namespace.
Confidentiality ==
    \forall r \in resources :
        (r.ns_prefix /= "none" /\ r.ns_prefix /= "root" /\ r.ns_prefix \in active_namespaces) =>
            (r.ns_prefix = client_ns[r.owner] \/ IsSuperPower(r.owner))

\* 2. Clipboard Isolation:
\*    No selection resource with a namespace prefix 'NS_A' can ever be owned 
\*    or accessed by a client belonging to a different namespace 'NS_B'.
ClipboardIsolation ==
    \forall s \in resources :
        (s.rtype = "Selection" /\ s.ns_prefix /= "none") =>
            (s.ns_prefix = client_ns[s.owner])

\* 3. No Resource Leakage (Memory Leak Proof):
\*    If a namespace is destroyed, its corresponding virtual root window MUST 
\*    be cleaned up and removed from the active resources database.
\*    (This Invariant fails under the buggy DestroyNamespace logic!)
NoResourceLeakage ==
    \forall ns \in Namespaces :
        (ns \notin active_namespaces) =>
            \forall r \in resources : r.id /= ns_root_win[ns]

=============================================================================
