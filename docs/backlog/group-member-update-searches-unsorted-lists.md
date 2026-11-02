---
worth: yes
where: src/server/core/userdb_objects.cpp:1691
added: 2026-11-02
---
# group member update searches lists it has unsorted

`Group::modifyFromMessage` and `Group::modifyFromJson` have two ordering defects in the same loop.

The old member list is searched while it is being overwritten. `*e = 0xFFFFFFFF` marks a found member, and the
next `FindMember` binary-searches the now unsorted array. With old and new lists both `[1,2,3]`, searching 3
probes the marked entries, goes left and misses. Unchanged member 3 counts as new and also as removed, so it
gets two redundant `USER_DB_MODIFY` notifications, sent with the user database write lock held.

Notifications go out before `m_members` is sorted. `SendUserDBUpdate` serializes the user on the spot
(`client.cpp:338`), and `FillGroupMembershipInfo` calls `isMember`, which binary-searches a partly filled, unsorted
`m_members`. Setting members of an empty group to `[3,2,1]` can send user 1 a notification that omits the group.
Nothing sends a correction after the sort, so `NXCSession` caches wrong membership for the user's Group
Membership page until the next update.

Fix: populate and sort `m_members` first, leave the old list untouched, then notify each old member missing from
the new list and each new member missing from the old one.

Same code on stable-6.2. Surfaced in review of #3861.
