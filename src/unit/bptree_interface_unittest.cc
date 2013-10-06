/*
    Copyright (C) 2013 University of Lugano

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "bptree_unittest.h"

class BptreeInterfaceTest : public BptreeTestBase {

protected:
    bptree_session *bps;
	tapioca_bptree_id tbpt_id;
	enum bptree_open_flags open_flags;
	
	int rv, rv1, rv2, keys;
	char k[LARGE_BUFFER_SZ];
	char v[LARGE_BUFFER_SZ];
	int32_t ksize, vsize;

    tapioca_handle* th;

	void insertSampleData() {
		
		strncpy(k, "a000", 5);
		strncpy(v, "v000", 5);
		for (int i = 1; i <= keys; i++)
		{
			sprintf(k,"a%03d",i);
			sprintf(v,"v%03d",i);
			rv = tapioca_bptree_insert(th, tbpt_id, &k, 5, &v, 5, BPTREE_INSERT_UNIQUE_KEY);
			ASSERT_EQ(rv, BPTREE_OP_SUCCESS);
			rv = tapioca_commit(th);
			EXPECT_GE(0, rv);
			if (i % 250 == 0) printf("Inserted %d keys\n", i);
		}

	}
	
	tapioca_bptree_id createNewTree(tapioca_bptree_id tbpt_id) {
		open_flags = BPTREE_OPEN_OVERWRITE;
		bps = (bptree_session *) malloc(sizeof(bptree_session));
		rv = tapioca_bptree_initialize_bpt_session(th, tbpt_id, open_flags);
		this->tbpt_id = tbpt_id;
		EXPECT_EQ(tbpt_id, rv);
	}
	
	virtual void SetUp() {
		system("killall -q -9 cm tapioca example_acceptor example_proposer rec");
		system("cd ..; bash scripts/launch_all.sh --kill-all --clear-db > /dev/null; cd -");
		sleep(1);
		memset(k, 0,LARGE_BUFFER_SZ);
		memset(v, 0,LARGE_BUFFER_SZ);
		keys = 1000;
		// Create a default tree for all test cases; some may create their own
        th = tapioca_open("127.0.0.1", 5555);
        EXPECT_NE(th, (tapioca_handle*)NULL);
		createNewTree(1000);
	}
	
	virtual void TearDown() {
        tapioca_close(th);
		system("killall -q cm tapioca example_acceptor example_proposer rec");
		sleep(2);
		system("killall -q -9 cm tapioca example_acceptor example_proposer rec");
	}

};


TEST_F(BptreeInterfaceTest, TestEmptyBptree)
{
	rv = tapioca_bptree_index_first(th, tbpt_id, k, &ksize, v, &vsize);

	EXPECT_EQ(rv, BPTREE_OP_EOF);
}

TEST_F(BptreeInterfaceTest, TestSingleElementTree)
{
	char kk[5] = "aaaa";
	char vv[5] = "cccc";
	vsize = 5;

	tapioca_bptree_set_num_fields(th,tbpt_id, 1);
	tapioca_bptree_set_field_info(th,tbpt_id, 0, 5, BPTREE_FIELD_COMP_STRNCMP);

	rv = tapioca_bptree_insert(th, tbpt_id, &kk, 5, &vv, 5, BPTREE_INSERT_UNIQUE_KEY);
	EXPECT_EQ(rv, BPTREE_OP_SUCCESS);

	rv = tapioca_commit(th);
	EXPECT_TRUE(rv >= 0);

	rv = tapioca_bptree_index_first(th, tbpt_id, k, &ksize, v, &vsize);
	EXPECT_EQ(rv, BPTREE_OP_KEY_FOUND);
	rv = tapioca_bptree_index_next(th, tbpt_id, k, &ksize, v, &vsize);
	EXPECT_EQ(rv, BPTREE_OP_EOF);
	// Did tapioca_bptree_first fetch the correct values?
	EXPECT_EQ(0, memcmp(k, kk, 5));
	EXPECT_EQ(0, memcmp(v, vv, 5));
}


TEST_F(BptreeInterfaceTest, TestUpdate)
{
	// Randomly sample from an array of size keys and insert into our btree
	int i, j, n, r;
	tapioca_bptree_set_num_fields(th, tbpt_id, 1);
	tapioca_bptree_set_field_info(th, tbpt_id, 0, 5, BPTREE_FIELD_COMP_STRNCMP);

	char v2[10] = "abcd12345";
	for (i = 1; i <= keys; i++)
	{
		sprintf(k,"%03d",i);
		rv = tapioca_bptree_insert(th, tbpt_id, k, 5, v, 10, BPTREE_INSERT_UNIQUE_KEY);
		ASSERT_EQ(rv, BPTREE_OP_SUCCESS);
		rv = tapioca_bptree_update(th, tbpt_id, k, 5, v2, 10);
		ASSERT_EQ(rv, BPTREE_OP_SUCCESS);
		rv = tapioca_commit(th);
		EXPECT_GE(0, rv);
		if (i % 250 == 0) printf("Updated %d keys\n", i);
	}
// TODO Add ordering check
/*	printf("Verifying ordering...OFF ");
	rv1 = verify_tapioca_bptree_order(th, tbpt_id, BPTREE_VERIFY_RECURSIVELY);
	//rv2 = verify_tapioca_bptree_order(th, tbpt_id, BPTREE_VERIFY_SEQUENTIALLY);
	dump_tapioca_bptree_contents(th, tbpt_id, 1, 0);
	return (rv1 && rv2);
	*/
}

TEST_F(BptreeInterfaceTest, MultiFieldInsertDupe) 
{
	tapioca_bptree_set_num_fields(th, tbpt_id, 4);
	tapioca_bptree_set_field_info(th, tbpt_id, 0, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);
	tapioca_bptree_set_field_info(th, tbpt_id, 1, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);
	tapioca_bptree_set_field_info(th, tbpt_id, 2, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);
	tapioca_bptree_set_field_info(th, tbpt_id, 3, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);
	int a, b, c, d;
	a = b = c = d = 1;
	memcpy(k, &a, 4);
	memcpy(k + 4, &b, 4);
	memcpy(k + 8, &c, 4);
	memcpy(k + 12, &d, 4);
	rv = tapioca_bptree_insert(th, tbpt_id, k, 16, v, 4, BPTREE_INSERT_UNIQUE_KEY);
	EXPECT_EQ(BPTREE_OP_SUCCESS, rv);
	d = 2;
	memcpy(k + 12, &d, 4);
	rv = tapioca_bptree_insert(th, tbpt_id, k, 16, v, 4, BPTREE_INSERT_UNIQUE_KEY);
	EXPECT_EQ(BPTREE_OP_SUCCESS, rv);
	d = 1;
	memcpy(k + 12, &d, 4);
	rv = tapioca_bptree_insert(th, tbpt_id, k, 16, v, 4, BPTREE_INSERT_UNIQUE_KEY);
	EXPECT_EQ(BPTREE_ERR_DUPLICATE_KEY_INSERTED, rv);
}

TEST_F(BptreeInterfaceTest, MultiFieldInsertUpdate) 
{
	char *kptr;
	tapioca_bptree_set_num_fields(th, tbpt_id, 4);
	tapioca_bptree_set_field_info(th, tbpt_id, 0, 5, BPTREE_FIELD_COMP_STRNCMP);
	tapioca_bptree_set_field_info(th, tbpt_id, 1, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);
	tapioca_bptree_set_field_info(th, tbpt_id, 2, 5, BPTREE_FIELD_COMP_STRNCMP);
	tapioca_bptree_set_field_info(th, tbpt_id, 3, sizeof(int32_t), BPTREE_FIELD_COMP_INT_32);

	for (int i = 1; i <= keys; i++)
	{
		sprintf(k, "a%03d", i);
		kptr = k;
		memcpy(kptr + 5, &i, sizeof(int32_t));
		memcpy(kptr + 9, k, 5);
		memcpy(kptr + 14, &i, sizeof(int32_t));

		rv = tapioca_bptree_insert(th, tbpt_id, k, 18, v, 5, BPTREE_INSERT_UNIQUE_KEY);
		EXPECT_EQ(rv, BPTREE_OP_SUCCESS);
		rv = tapioca_commit(th);
		EXPECT_GE(0, rv);
		if (i % 250 == 0) printf("Inserted %d keys\n", i);
	}

	char v2[10] = "abcd12345";
	for (int i = 1; i <= keys; i++)
	{
		sprintf(k, "a%03d", i);
		kptr = k;
		memcpy(kptr + 5, &i, sizeof(int32_t));
		memcpy(kptr + 9, k, 5);
		memcpy(kptr + 14, &i, sizeof(int32_t));

		rv = tapioca_bptree_update(th, tbpt_id, k, 18, v2, 10);
		ASSERT_EQ(rv, BPTREE_OP_SUCCESS);
		rv = tapioca_commit(th);
		ASSERT_GE(0, rv);
		if (i % 250 == 0) printf("Updated %d keys\n", i);
	}

	//printf("Verifying ordering... OFF");
//	rv1 = verify_tapioca_bptree_order(th, tbpt_id, BPTREE_VERIFY_RECURSIVELY);
//	rv2 = verify_tapioca_bptree_order(th, tbpt_id, BPTREE_VERIFY_SEQUENTIALLY);
//	dump_tapioca_bptree_contents(th, tbpt_id, 1, 1);
}