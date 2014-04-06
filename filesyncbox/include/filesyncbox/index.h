#pragma once

/**
 * Index keeps all chunks in a multimap by all used hashes.
 * It can quickly find a chunk given any hash.
 *
 * Index also allows to look up hash information for a given local file, under any directory.
 * It will calculate the information upon first request and cache it.

multimap<inner_hash, outer_hash, small_hash => chunk*>

 */
