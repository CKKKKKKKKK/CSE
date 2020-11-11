# Reading 2

程可

518021910095



### 1. What are the roles that a node can have in Raft? What is the function of each role?

​		Leader, follower and candidate.

​		Leader's functions:

- Send heartbeat to each server, repeat during idle periods to prevent election timeout

- If command received from client, then append entry to local log, respond after entry applied to state machine

- If last log index ≥ nextIndex for a follower, then send AppendEntries RPC with log entries starting at nextIndex. If successful, update nextIndex and matchIndex for follower. If AppendEntries fails because of log inconsistency, decrement nextIndex and retry

- If there exists an N such that N > commitIndex, a majority of matchIndex[i] ≥ N, and log[N].term == currentTerm, then set commitIndex = N 

  

  Follower's functions:

- Respond to RPCs from candidates and leaders

- If election timeout elapses without receiving AppendEntries RPC from current leader or granting vote to candidate then convert to candidate



​		Candidate's functions:

- On conversion to candidate, start election which includes four steps: increment currentTerm , vote for self, reset election timer and send RequestVote RPCs to all other servers
- If votes received from majority of servers: become leader
- If AppendEntries RPC received from new leader then it is converted to follower
- If election timeout elapses then start new election	



### 2. What is the log for?

​		 For a single server, the log is used to achieve all-or-nothing property: if a transaction has been commited in the log, then whenever the server crashes and recovers, it can redo the commited transactions in the log to recover the results. For a collection of servers described in Raft, the log can also ensure the all-or-nothing property of the whole system because of the election restriction. Moreover, for a collection of servers in Raft, the log can be used to help ahieve consensus among servers because the state machines are deterministic and given the same log they can compute the same state, thus can achieve the consensus among the servers.



### 3. How does a cluster of nodes elect a leader?

- Start of the election

  ​		Raft uses a heartbeat mechanism to trigger leader election. If a follower receives no communication over a period of time, then it assumes there is no viable leader and begins an election to choose a new leader.

- Election

  ​		To begin an election, a follower increments its current term and transitions to candidate state. It then votes for itself and issues RequestVote RPCs in parallel to each of the other servers in the cluster.  And each server will vote for at most one candidate in a given term on a first-come-first-served basis. Moreover, according to the  A candidate continues in this state until one of three things happens: it wins the election, or another server establishes itself as leader, or a period of time goes by with no winner. 

- Result of the election

  ​	A candidate wins an election if it receives votes from a majority of the servers in the full cluster for the same term.  Once a candidate wins an election, it becomes leader. It then sends heartbeat messages to all of the other servers to establish its authority and prevent new elections.

  ​	While waiting for votes, a candidate may receive an AppendEntries RPC from another server claiming to be leader. If the leader’s term is at least as large as the candidate’s current term, then the candidate recognizes the leader as legitimate and returns to follower state. If the term in the RPC is smaller than the candidate’s current term, then the candidate rejects the RPC and continues in candidate state.

  ​	The third possible outcome is that a candidate neither wins nor loses the election: if many followers become candidates at the same time, votes could be split so that no candidate obtains a majority. When this happens, each candidate will time out and start a new election by incrementing its term and initiating another round of RequestVote RPCs. 

  

### 4. "Without a steady leader, Raft cannot make progress."  Why not?

​		Because all requests from the clients are sent to the leader. If there is no steady leader, then there will be frequent election, which means the whole system will be unavailable for the most of time because the servers are "busy with" election and will not respond to the clients' requests.



### 5. Why is it important that at most one candidate win an election (the Election Safety Problem)?

​		Because if more than one candidates win an election, there will be more than one leaders at the same time, then the whole system cannot reach consensus.



### 6. How does Raft handle follower failures?

 		If a follower crashes, then future RequestVote and AppendEntries RPCs sent to it will fail. Raft handles these failures by retrying indefinitely; if the crashed server restarts, then the RPC will complete successfully. If a server crashes after completing an RPC but before responding, then it will receive the same RPC again after it restarts. Raft RPCs are idempotent, so this causes no harm. And if the follower miss some AppendEntries RPCs because of crash, then after it restarts and receive an AppendEntries RPC from the leader,  the leader will find that the follower’s log is inconsistent with the leader’s, the AppendEntries consistency check will fail in the AppendEntries RPC. After a rejection, the leader decrements nextIndex and retries the AppendEntries RPC. Eventually nextIndex will reach a point where the leader and follower logs match and the entries the follower has missed will be appended.



### 7. How does Raft handle candidate failures?

 		If a candidate crashes, then future RequestVote and AppendEntries RPCs sent to it will fail. Raft handles these failures by retrying indefinitely; if the crashed server restarts, then the RPC will complete successfully. If a server crashes after completing an RPC but before responding, then it will receive the same RPC again after it restarts. Raft RPCs are idempotent, so this causes no harm. 



### 8. How does Raft handle leader failures?

​		If a leader crashes, then there will be election timeout for some followers and these followers will become candidates in a new term of election. The election will repeat until only one candidate win the election. And because of the Election Restriction, the new leader will have all the entries the old leader has committed and this ensure the atomicity of the whole system.



### 9. How does Raft handle network partitions?

​		If the leader can still connect to a majority of the servers, then although the rest servers cannot hear the heartbeat of the leader and some of them become candidates, because the current leader still "govern" a majority of the servers, none of the candidates can receive votes from a majority of servers to become a leader. So the election started by these "minority" servers will go on infinitely until the candidates connect to the current leader again and because it can receive the heartbeat of the leader before the election timeout elapses, it will step down to become a follower again.

​		If the leader cannot connect to a majority of the servers, then it cannot commit any updates requested by the client because it cannot replicate the log entries on a majority of the servers. At the same time, those servers which cannot hear the heartbeats from the leader and some of them whose election timeouts elapse, they will become candidates. If a candidate can receive votes from a majority of the servers, then it will become the new leader and any request from the client sent to it can be committed because it can replicate log entries on a majority of servers. And when the network partition heal, the old leader can connect to the new leader, the old leader will observe a larger term number and step down to become a follower of the new leader. On the other hand, if none of the candidates can receive majority votes, then their election will go on and the whole system is not available until the current leader can connect to a majority of the servers or one of the candidate can receive majority votes.



### 10. At the top of 5.3, the paper says that leaders will retry AppendEntries RPCs indefinitely. Why don't they abort after a timeout?

​		If the leader can't receive reponse to its AppendEntries RPCs from some followers and there is a timeout, there are two cases:

​		First, if the leader has already replicated the entries on a majority of the servers, it will reply committed to the client, so it is impossible for the leader to abort the committed entries.

​		Second, if the leader has not yet replicated the entries on a majority of the servers, then it just retry AppendEntries RPCs after a timeout because it can see that nextIndexes of those servers are smaller than the last log index. As for why not the leader abort the entries, if these servers are just slow followers, just after the leader decides to abort the slow followers reply to the previous AppendEntries RPCs and now there are a majority of the servers reply to the previous AppendEntries RPCs, what should the leader do? Choose to reply committed to the client? But it has already decided to abort and notified other servers through RPCs. Choose to abort? What if the slow followers reply the abort RPCs after timeout again? Abort the abort? So abort may incur complication and affect consensus, and the better choice is to retry AppendEntries RPCs indefinitely.