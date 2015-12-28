/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QCoreApplication>

#include <Libraries/PrlNetworking/prl_offmgmt.h>
#include <prlcommon/Std/PrlAssert.h>
#include <sys/socket.h>

using namespace om;

int srv()
{
	// Try to open and close
	void *db1;
	PRL_RESULT res;

	// keep db1 opened
	res = ruledb::open(&db1, true);
	PRL_ASSERT(0 == res);

	void *db;

	// reopen
	res = ruledb::open(&db, true);
	PRL_ASSERT(0 == res);

	// Add entries
	PRL_ASSERT(0 == ruledb::get_count(db));

	ruledb::rule r;
	memset(&r, 0, sizeof(r));
	r.sa.sa_family = PRL_AF_INET;

	res = ruledb::add_rule(db, &r);
	PRL_ASSERT(0 == res);

	PRL_ASSERT(1 == ruledb::get_count(db));

	// port-entry and uuid-entry should be ignored on add
	r.dst_port++;
	r.uuid[0]++;
	res = ruledb::add_rule(db, &r);
	PRL_ASSERT(PRL_ERR_ENTRY_ALREADY_EXISTS == res);
	PRL_ASSERT(1 == ruledb::get_count(db));

	// When AF is PRL_AF_INET, only first dword of addr should
	// be taken into account
	r.sa.sa_family = PRL_AF_INET;
	r.sa.addr[1]++;
	res = ruledb::add_rule(db, &r);
	PRL_ASSERT(PRL_ERR_ENTRY_ALREADY_EXISTS == res);
	PRL_ASSERT(1 == ruledb::get_count(db));

	ruledb::close(db);

	// check that create makes entries be 0
	res = ruledb::open(&db, true);
	PRL_ASSERT(0 == res);
	PRL_ASSERT(0 == ruledb::get_count(db));

	r.sa.sa_family = PRL_AF_INET;
	r.sa.addr[0] = 0;
	res = ruledb::add_rule(db, &r);
	PRL_ASSERT(0 == res);

	for (int i = 0; i<32; ++i) {
		r.sa.addr[0]++;
		res = ruledb::add_rule(db, &r);
		PRL_ASSERT(0 == res);
	}

	PRL_ASSERT(33 == ruledb::get_count(db));

	// remove entry in the middle
	r.sa.addr[0] = 10;
	res = ruledb::remove_rule(db, &r);
	PRL_ASSERT(0 == res);
	PRL_ASSERT(32 == ruledb::get_count(db));

	prl_sockaddr r1;
	r1.sa_family = PRL_AF_INET;
	r1.addr[0] = 0;
	r1.port = 0;
	for (int i = 0; i<33; ++i) {
		r1.addr[0] = i;
		res = find(db, &r1, &r);
		if (i == 10) {
			PRL_ASSERT(res==PRL_ERR_FILE_NOT_FOUND);
		}
		else {
			PRL_ASSERT(res == 0);
		}
	}

	// add some enries for VMs
	memset(r.uuid, 0, sizeof(r.uuid));
	for (int i = 0; i<32; ++i) {
		r.uuid[i%16] = i + 2;
		r.sa.addr[0] = 64 + i;
		res = ruledb::add_rule(db, &r);
		PRL_ASSERT(0 == res);
	}

	PRL_ASSERT(64 == ruledb::get_count(db));

	// remove entries for VM with uuid 1
	prl_uuid_t u1;
	memset(u1, 0, sizeof(u1));
	u1[0] = 1;
	res = om::ruledb::remove_vm(db, u1);
	PRL_ASSERT(0 == res);

	// dump the db
	res = ruledb::dump(db);
	PRL_ASSERT(res == 0);

	PRL_ASSERT(32 == ruledb::get_count(db));

	// dump the db
	res = ruledb::dump(db);
	PRL_ASSERT(res == 0);

	// Wait for client connected
	printf("Press any key when client-tests done\n");
	getchar();

	// try to add as many rules as possible
	res = ruledb::clear(db);
	PRL_ASSERT(res == 0);

	for (int i = 0;;++i) {
		r.sa.addr[0] = i;
		res = ruledb::add_rule(db, &r);
		if (res == PRL_ERR_BUFFER_OVERRUN) {
			printf("entries are in buffer: %d\n", ruledb::get_count(db));
			break;
		}
		PRL_ASSERT(0 == res);
	}

	ruledb::close(db1);
	ruledb::close(db);

	return 0;
}


int client()
{
	void *db;
	PRL_RESULT res;

	res = ruledb::open(&db, false);
	PRL_ASSERT(0 == res);

	// dump the db
	res = ruledb::dump(db);
	PRL_ASSERT(res == 0);

	ruledb::close(db);

	return 0;
}


int main(int argc, char *argv[])
{
	QCoreApplication app( argc, argv);
	if (argc<2 || 0 == strcmp(argv[1], "srv"))
		return srv();
	return client();
}
