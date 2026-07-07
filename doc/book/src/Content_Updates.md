# Content Updates

## Overview

In the Wayland protocol, requests are asynchronous but take effect immediately
when the compositor receives them. However, some requests on surfaces are not
applied immediately but are instead double-buffered to allow atomic changes.
These double-buffered changes are committed through the wl_surface.commit
request, which creates a Content Update.

Content Updates encapsulate all double-buffered state changes and can be applied
by the compositor. The complexity arises when considering subsurfaces, which can
operate in synchronized mode. When a subsurface is synchronized, its Content
Updates must be applied atomically together with the parent surface's state.
This synchronization can extend through an entire tree of subsurfaces, where
child subsurfaces inherit the synchronized behavior from their parents.

Historically, Content Updates from synchronized subsurfaces were merged into the
pending state of the parent surface on commit. However, the introduction of
constraints—which can defer the application of Content Updates—necessitated a
more sophisticated model. This led to the implementation of per-surface queues
of Content Updates, with dependencies between Content Updates across different
queues. This queuing model maintains backwards compatibility with the earlier
approach of merging Content Updates into the parent's pending state on commit.

The core protocol defines the semantics of Content Updates using per-surface
queues, but compositors that do not need to support constraints may implement
the simpler legacy model where synchronized subsurface states are merged
directly into the parent's pending state.

## Rules

The core protocol specifies the behavior in wl_subsurface and wl_surface.commit.
The behavior can be summarized by the following rules:

1. Content Updates (CU) contain all double-buffered state of the surface and
   selected state from their direct children.

2. Surfaces which are effectively synchronized create Synchronized Content
   Updates (SCU), otherwise they create Desync Content Updates (DCU).

3. When a CU is created, it gets a dependency on the previous CU of the same
   queues (if it exists).

4. When a CU is created, it gets a dependency on the last SCU of direct child
   surfaces that are not reachable (if they exists).

5. The CUs and their dependencies form a DAG, where CUs are nodes and
   dependencies are edges.

6. All DCUs starting from the front of the queues until the first SCU or the
   back of the queue is reached are candidates.

7. If the maximal DAG that's reachable from a candidate (candidate DAG) does not
   have any constraints, then this DAG can be applied.

8. A DAG is applied atomically by recursively applying a content update without
   dependencies and removing it from the DAG.

9. Surfaces transition from effectively sync to effectively desync after their
   parents.

10. When a surface transitions to effectively desync, all SCUs in its queue
    which are not reachable by a DCU become DCUs.

## Examples

These examples should help to build an intuition for how content updates
actually behave. They cover the interesting edge cases, such as subsurfaces with
constraints, and transitioning from a sync subsurface to a desync one.

In all the examples below, the surface T1 refers to a toplevel surface, SS1
refers to a sub-surface which is a child of T1, and SS2 refers to a sub-surface
which is a child of SS1.

### Legend

![](images/content-updates/content-update-legend.png)

### Simple Desynchronized Case

1. SS2 is effectively desynchronized and commits. This results in the
   desynchronized content update (DCU) _1_.

   ![](images/content-updates/simple-desynchronized-state-1.png)

2. DCU _1_ is a candidate, and the candidate DAG reachable from DCU _1_ is only DCU
   _1_ itself. DCU _1_ and thus the candidate DAG does not have any constraints and
   can be applied.

   ![](images/content-updates/simple-desynchronized-state-2.png)

3. The content updates of the candidate DAG get applied to the surface atomically.

   ![](images/content-updates/simple-desynchronized-state-3.png)

4. T1 commits a DCU with a _buffer-sync_ constraint. It is a candidate but its DAG
   can't be applied because it contains a constraint.

   ![](images/content-updates/simple-desynchronized-state-4.png)

5. T1 commits another CU (DCU _3_) which is added at the end of the queue, with a
   dependency to the previous CU (DCU _2_). Both DCU _2_ and DCU _3_ are
   candidates, but both DAGs contain DCU _2_ with a constraint, and can't be
   applied.

   ![](images/content-updates/simple-desynchronized-state-5.png)

6. When the constraint gets cleared, both DAGs can be applied to the surface
   atomitcally (either only _2_, or _2_ and _3_).

   ![](images/content-updates/simple-desynchronized-state-6.png)

### Simple Synchronized Case

1. SS1 and SS2 are effectively synchronized. SS2 commits SCU _1_.

   ![](images/content-updates/simple-synchronized-state-1.png)

2. SS1 commits SCU _2_. The direct child surfaces SS2 has the last SCU _1_ in its
   queue, which is not reachable. This creates a dependency from SCU _2_ to SCU
   _1_.

   ![](images/content-updates/simple-synchronized-state-2.png)

3. SS1 commits SCU _3_. The direct child surfaces SS2 has the last SCU _1_ in its
   queue, which is already reachable by SCU _2_. No dependency to SCU _1_ is
   created. A dependency to the previous CU of the same queue (SCU _2_) is created.

   ![](images/content-updates/simple-synchronized-state-3.png)

4. T1 commit DCU _4_. It is a candidate, its DAG does not contain any constraint
   and it can be applied.

   ![](images/content-updates/simple-synchronized-state-4.png)

5. The DAG gets applied to the surfaces atomically.

   ![](images/content-updates/simple-synchronized-state-5.png)

### Complex Synchronized Subsurface Case 1

1. Every DCU (_1_ and _6_) contain CUs with constraints in their candidate DAG

   ![](images/content-updates/sync-subsurf-case1-1.png)

2. Waiting until the _buffer-sync_ constrain on CU _1_ is cleared, the candidate
   DAG of CU _1_ does not contain constraints and can be applied

   ![](images/content-updates/sync-subsurf-case1-2.png)

3. That leaves the candidate DAG of CU _6_ which still contains another CU with a
   _buffer-sync_ constrain

   ![](images/content-updates/sync-subsurf-case1-3.png)

4. Waiting until the _buffer-sync_ constrain on CU _6_ is cleared, the candidate
   DAG of _6_ does not contain CUs with constraints and can be applied.

   ![](images/content-updates/sync-subsurf-case1-4.png)

5. There is no DCU left and no constraint remaining. Nothing more can be applied
   without a new CU.

   ![](images/content-updates/sync-subsurf-case1-5.png)

### Complex Synchronized Subsurface Case 2

1. Both DCUs (_1_ and _6_) have a reachable DAG containing CU _1_ with a constraint

   ![](images/content-updates/sync-subsurf-case2-1.png)

2. Waiting until the _buffer-sync_ constrain on _1_ is cleared, both DAGs contain
   no CU with constraints and can be applied in any order

   ![](images/content-updates/sync-subsurf-case2-2.png)

3. That leaves the same state as in the previous case

   ![](images/content-updates/sync-subsurf-case2-3.png)

### Synchronized to Desynchronized Subsurface

1. There is one DCU (_4_) with its reachable DAG that cannot be applied because CU
   _4_ contains a constraint

   ![](images/content-updates/sync-to-desync-subsurf-1.png)

2. Surface _SS1_ transitions from effectively synchronized to effectively
   desynchronized. SCU _2_ is reachable by DCU _4_ so nothing changes.

   ![](images/content-updates/sync-to-desync-subsurf-2.png)

3. Surface _SS1_ provides a new DCU (_5_) but because the CU before (_2_) is a
   Synchronized CU, it is not a candidate

   ![](images/content-updates/sync-to-desync-subsurf-3.png)

### Synchronized to Desynchronized Transition

1. There are four SCUs and all surfaces are effectively synchronized.

   ![](images/content-updates/sync-to-desync-transition-1.png)

2. Surface _SS1_ transitions to effectively desynchronized and SCU _2_ becomes a
   DCU because it is not reachable from a DCU

   ![](images/content-updates/sync-to-desync-transition-2.png)

3. Surface _SS2_ transitions to effectively desynchronized. SCUs _3_ and _4_ become
   DCUs because they are not reachable from a DCU. SCU _1_ does not change because
   it is reachable by DCU _2_.

   ![](images/content-updates/sync-to-desync-transition-3.png)
