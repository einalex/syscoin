// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "asset.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <chrono>
#include "ranges.h"
using namespace boost::chrono;
using namespace std;
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

void printRangeVector (vector<CRange> &vecRange, string tag) {
	printf("Printing vector range %s: ", tag.c_str());
	for(int index = 0; index < vecRange.size(); index++) {
		printf("{%i,%i} ", vecRange[index].start, vecRange[index].end);
	}
	printf("\n");
}
void addToRangeVector (vector<CRange> &vecRange, int range_single) { 
	CRange range(range_single, range_single);
	vecRange.push_back(range);
}
void addToRangeVector (vector<CRange> &vecRange, int range_start, int range_end) { 
	CRange range(range_start, range_end);
	vecRange.push_back(range);
}

BOOST_FIXTURE_TEST_SUITE (syscoin_asset_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE(generate_range_merge)
{
	printf("Running generate_range_merge...\n");
	// start with {0,0} {2,3} {6,8}, add {4,5} to it and expect {0,0} {2,8}
	CheckRangeMerge("{0,0} {2,3} {6,8}", "{4,5}", "{0,0} {2,8}");

	CheckRangeMerge("{0,0} {2,3} {6,8}", "{4,5}", "{0,0} {2,8}");
	CheckRangeMerge("{2,3} {6,8}", "{0,0} {4,5}", "{0,0} {2,8}");
	CheckRangeMerge("{2,3}", "{0,0} {4,5} {6,8}", "{0,0} {2,8}");
	CheckRangeMerge("{0,0} {4,5} {6,8}", "{2,3}", "{0,0} {2,8}");

	CheckRangeMerge("{0,0} {2,2} {4,4} {6,6} {8,8}", "{1,1} {3,3} {5,5} {7,7} {9,9}", "{0,9}");
	CheckRangeMerge("{0,8}","{9,9}","{0,9}");
	CheckRangeMerge("{0,8}","{10,10}","{0,8} {10,10}");
	CheckRangeMerge("{0,0} {2,2} {4,4} {6,6} {8,8} {10,10} {12,12} {14,14} {16,16} {18,18} {20,20} {22,22} {24,24} {26,26} {28,28} {30,30} {32,32} {34,34} {36,36} {38,38} {40,40} {42,42} {44,44} {46,46} {48,48}", "{1,1} {3,3} {5,5} {7,7} {9,9} {11,11} {13,13} {15,15} {17,17} {19,19} {21,21} {23,23} {25,25} {27,27} {29,29} {31,31} {33,33} {35,35} {37,37} {39,39} {41,41} {43,43} {45,45} {47,47} {49,49}", "{0,49}");  

}
BOOST_AUTO_TEST_CASE(generate_range_subtract)
{
	printf("Running generate_range_subtract...\n");
	// start with {0,9}, subtract {0,0} {2,3} {6,8} from it and expect {1,1} {4,5} {9,9}
	CheckRangeSubtract("{0,9}", "{0,0} {2,3} {6,8}", "{1,1} {4,5} {9,9}");

	CheckRangeSubtract("{1,2} {3,3} {6,10}", "{0,0} {2,2} {3,3}", "{1,1} {6,10}");
	CheckRangeSubtract("{1,2} {3,3} {6,10}", "{0,0} {2,2}", "{1,1} {3,3} {6,10}");
}
BOOST_AUTO_TEST_CASE(generate_range_contain)
{
	printf("Running generate_range_contain...\n");
	// does {0,9} contain {0,0}?
	BOOST_CHECK(DoesRangeContain("{0,9}", "{0,0}"));
	BOOST_CHECK(DoesRangeContain("{0,2}", "{1,2}"));
	BOOST_CHECK(DoesRangeContain("{0,3}", "{2,2}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{2,2}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{2,3}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{6,7}"));
	BOOST_CHECK(DoesRangeContain("{0,8}", "{0,0} {2,3} {6,8}"));
	BOOST_CHECK(DoesRangeContain("{0,8}", "{0,1} {2,4} {6,6}"));

	BOOST_CHECK(!DoesRangeContain("{1,9}", "{0,0}"));
	BOOST_CHECK(!DoesRangeContain("{1,9}", "{1,10}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{1,3}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{0,2}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{0,3}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{1,2}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{0,1}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{4,4}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{4,5}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{0,8}"));
	BOOST_CHECK(!DoesRangeContain("{0,8}", "{0,1} {2,4} {6,9}"));
	BOOST_CHECK(!DoesRangeContain("{0,8}", "{0,9} {2,4} {6,8}"));
}
BOOST_AUTO_TEST_CASE(generate_range_complex)
{
	/* Test 1:  Generate two large input, 1 all even number 1 all odd and merge them */
	/* This test uses Range Test Library that contains addition vector operations    */
	printf("Running generate_range_complex...\n");
	string input1="", input2="", expected_output="";
	int total_range = 10000;
	int64_t ms1 = 0, ms2 = 0;

	printf("ExpectedOutput: range from 0-%i\n", total_range);
	printf("Input1: range from 0-%i\n Even number only\n", total_range-2);
	printf("Input2: range from 1-%i\n Odd number only\n", total_range-1);

	for (int index = 0; index < total_range; index=index+2) {
		input1 = input1 + "{" + to_string(index) + "," + to_string(index) +"} ";
		input2 = input2 + "{" + to_string(index+1) + "," + to_string(index+1) +"} ";
	}
	expected_output = "{0," + to_string(total_range-1) + "}"; 
	// Remove the last space from the string
	input1.pop_back();
	input2.pop_back();
	printf("Rangemerge Test: input1 + input2 = ExpectedOutput\n");
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeMerge(input1, input2, expected_output);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeMerge Completed %ldms\n", ms2-ms1);

	/* Test 2: Reverse of Test 1 (expected_output - input = 2) */
 	printf("RangeSubstract Test: ExpectedOutput - input1 = input2\n");	
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeSubtract(expected_output, input1, input2);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeSubtract1 Completed %ldms\n", ms2-ms1);

 	printf("RangeSubstract Test: ExpectedOutput - input2 = input1\n");	
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeSubtract(expected_output, input2, input1);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeSubtract2 Completed %ldms\n", ms2-ms1);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_merge1) 
{
	// Test: merge1 
	// range1: {0-10m} even only
	// range2: {1-9999999} odd only
	// output:  range1 + range2
	printf("Running generate_range_stress_merge1:...\n");
	int total_range = 10000000;
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o, vecRange_expected;
	int64_t ms1 = 0, ms2 = 0;

	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index, index);
		addToRangeVector(vecRange2_i, index+1, index+1);
	}
	
	// Set expected outcome
	addToRangeVector(vecRange_expected, 0, total_range-1);
	
	// combine the two vectors of ranges
	vecRange1_i.insert(std::end(vecRange1_i), std::begin(vecRange2_i), std::end(vecRange2_i));

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	mergeRanges(vecRange1_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	printf("\noutput range 1+2: merge time: %ldms\n", ms2-ms1);

	BOOST_CHECK(vecRange_o.size() == vecRange_expected.size());
	BOOST_CHECK(vecRange_o.back() == vecRange_expected.back());
	printf("\n Stress Test done \n");

}
BOOST_AUTO_TEST_CASE(generate_range_stress_subtract1) 
{
	// Test: subtract1 
	// range1: {0-1m} 
	// range2: {1m, 4m ,7m}
	// output:  range1 - range2
	printf("Running generate_range_stress_subtract1...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o, vecRange_expected;
	vector<CRange> vecRange2_i_copy;
	int64_t ms1 = 0, ms2 = 0;

	
	// Set input range 1 {0,10m}
	addToRangeVector(vecRange1_i, 0, 10000000);

	printRangeVector(vecRange1_i, "vecRange 1 input");
	// Set input range 2 {1m,4m,7m}
	addToRangeVector(vecRange2_i, 1000000);
	addToRangeVector(vecRange2_i, 4000000);
	addToRangeVector(vecRange2_i, 7000000);
	printRangeVector(vecRange2_i, "vecRange 2 input");
	
	// Set expected output {(1-999999),(1000001-3999999)...}
	addToRangeVector(vecRange_expected, 0, 999999);
	addToRangeVector(vecRange_expected, 1000001, 3999999);
	addToRangeVector(vecRange_expected, 4000001, 6999999);
	addToRangeVector(vecRange_expected, 7000001, 10000000);
	printRangeVector(vecRange_expected, "vecRange_expected");
	
	// Deep copy for test #2 since the vector will get modified
	vecRange2_i_copy = vecRange2_i;
	

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	subtractRanges(vecRange1_i, vecRange2_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	BOOST_CHECK(vecRange_o.size() == vecRange_expected.size());
	BOOST_CHECK(vecRange_o.back() == vecRange_expected.back());

	vecRange2_i = vecRange2_i_copy;
	vector<CRange> vecRange2_o;
	vecRange2_i.insert(std::end(vecRange2_i), std::begin(vecRange_expected), std::end(vecRange_expected));
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	mergeRanges(vecRange2_i, vecRange2_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("\noutput range expected+2: merge time: %ld\n", ms2-ms1);

	BOOST_CHECK_EQUAL(vecRange2_o.size(), vecRange1_i.size());
	BOOST_CHECK_EQUAL(vecRange2_o.back().start, 0);
	BOOST_CHECK_EQUAL(vecRange2_o.back().end, 10000000);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_merge2) 
{
	// Test: merge2
	// range1: {0-1m} odd only
	// range2: {100000 200000, ..., 900000 }
	// output:  range1 + range2
	printf("Running generate_range_stress_merge2...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o;
	int64_t ms1 = 0, ms2 = 0;
	int total_range = 1000000;

	// Create vector range 1 that's 0-1mill odd only
	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index+1, index+1);
	}
	// Create vector range 2 that's 100k,200k,300k...,900k
	for (int index = 100000; index < total_range; index=index+100000) {
		addToRangeVector(vecRange2_i, index, index);
	}

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	vecRange1_i.insert(std::end(vecRange1_i), std::begin(vecRange2_i), std::end(vecRange2_i));
	mergeRanges(vecRange1_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	//HARDCODED checks
	BOOST_CHECK_EQUAL(vecRange_o.size(), (total_range/2) - 9);
	BOOST_CHECK_EQUAL(vecRange_o[49999].start, 99999); 
	BOOST_CHECK_EQUAL(vecRange_o[49999].end, 100001); 
	BOOST_CHECK_EQUAL(vecRange_o[99998].start, 199999); 
	BOOST_CHECK_EQUAL(vecRange_o[99998].end, 200001); 
	BOOST_CHECK_EQUAL(vecRange_o[449991].start, 899999); 
	BOOST_CHECK_EQUAL(vecRange_o[449991].end, 900001); 
	printf("CheckRangeSubtract Completed %ldms\n", ms2-ms1);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_subtract2) 
{
	// Test: subtract2
	// range1: {0-1m} odd only
	// range2: {100001 200001, ..., 900001 }
	// output:  range1 - range2
	printf("Running generate_range_stress_subtract3...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o;
	int64_t ms1 = 0, ms2 = 0;
	int total_range = 1000000;

	// Create vector range 1 that's 0-1mill odd only
	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index+1,index+1);
	}
	// Create vector range 2 that's 100k,200k,300k...,900k
	for (int index = 100000; index < total_range; index=index+100000) {
		addToRangeVector(vecRange2_i, index+1,index+1);
	}

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	subtractRanges(vecRange1_i, vecRange2_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	//HARDCODED checks
	BOOST_CHECK_EQUAL(vecRange_o.size(), 499991);
	printf("CheckRangeSubtract Completed %ldms\n", ms2-ms1);
}

BOOST_AUTO_TEST_CASE(generate_big_assetdata)
{
	ECC_Start();
	GenerateSpendableCoins();
	printf("Running generate_big_assetdata...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetbig1", "data");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 257 bytes long
	UniValue r;
	string baddata = gooddata + "a";
	string guid = AssetNew("node1", "chf", "jagassetbig1", gooddata);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "scanassets"));
	UniValue rArray = r.get_array();
	BOOST_CHECK(rArray.size() > 0);
	BOOST_CHECK_EQUAL(find_value(rArray[0].get_obj(), "_id").get_str(), guid);
	string guid1 = AssetNew("node1", "usd", "jagassetbig1", gooddata);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "symbol").get_str() == "CHF");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1 + " false"));
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid1);
	BOOST_CHECK(find_value(r.get_obj(), "symbol").get_str() == "USD");
}
/*BOOST_AUTO_TEST_CASE(generate_asset_throughput)
 {
	UniValue r;
	printf("Running generate_asset_throughput...\n");
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node3");
	map<string, string> assetMap;
	map<string, string> assetAliasMap;
	AliasNew("node1", "fundingtps", "data");
	AliasNew("node3", "fundingtps3", "data");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress fundingtps 200000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress fundingtps3 200000"), runtime_error);
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node3");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo fundingtps"));
	string strAddress = find_value(r.get_obj(), "address").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo fundingtps3"));
	string strAddress3 = find_value(r.get_obj(), "address").get_str();
	// create 1000 aliases and assets for each asset	
	printf("creating sender 1000 aliases/asset...\n");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	int64_t mediantime = find_value(r.get_obj(), "mediantime").get_int64();
	mediantime += ONE_YEAR_IN_SECONDS;
	string mediantimestr = boost::lexical_cast<string>(mediantime);
	for (int i = 0; i < 1000; i++) {
		string aliasname = "jagthroughput" + boost::lexical_cast<string>(i);
		string aliasnameto = "jagthroughputto" + boost::lexical_cast<string>(i);
		
		// registration	
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew " + aliasname + " '' 3 " + mediantimestr + " '' '' '' ''"));
		UniValue varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscointxfund " + varray[0].get_str() + " " + "\"{\\\"addresses\\\":[\\\"" + strAddress + "\\\"]}\""));
		varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + varray[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		// activation	
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew " + aliasname + " '' 3 " + mediantimestr + " '' '' '' ''"));
		UniValue varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscointxfund " + varray1[0].get_str() + " " + "\"{\\\"addresses\\\":[\\\"" + strAddress + "\\\"]}\""));
		varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + varray1[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew tpstest " + aliasname + " '' " + " assets " + " 8 false 1 10 0 false ''"));
		UniValue arr = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
		string hex_str = find_value(r.get_obj(), "hex").get_str();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
		
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		string guid = arr[1].get_str();
		
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetsend " + guid + " " + aliasname + " " + "\"[{\\\"aliasto\\\":\\\"" + aliasname + "\\\",\\\"amount\\\":1}]\" '' ''"));
		arr = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
		hex_str = find_value(r.get_obj(), "hex").get_str();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		
		assetMap[guid] = aliasnameto;
		assetAliasMap[guid] = aliasname;
		if (i % 100 == 0)
			 printf("%.2f percentage done\n", 100.0f / (1000.0f / (i + 1)));
		
	}
	printf("creating receiver 1000 aliases/asset...\n");
	int count = 0;
	for (auto& assetTuple : assetMap) {
		count++;
		string aliasname = assetTuple.second;
		
		// registration	
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasnew " + aliasname + " '' 3 " + mediantimestr + " '' '' '' ''"));
		UniValue varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscointxfund " + varray[0].get_str() + " " + "\"{\\\"addresses\\\":[\\\"" + strAddress3 + "\\\"]}\""));
		varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "signrawtransaction " + varray[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "generate 1"));
		// activation	
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasnew " + aliasname + " '' 3 " + mediantimestr + " '' '' '' ''"));
		UniValue varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscointxfund " + varray1[0].get_str() + " " + "\"{\\\"addresses\\\":[\\\"" + strAddress3 + "\\\"]}\""));
		varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "signrawtransaction " + varray1[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "generate 1"));
		if (count % 100 == 0)
			 printf("%.2f percentage done\n", 100.0f / (1000.0f / count));
	}
	GenerateBlocks(10);
	printf("Creating assetsend transactions to node3 alias...\n");
	vector<string> assetSendTxVec;
	count = 0;
	for (auto& assetTuple : assetMap) {
		count++;
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsend " + assetTuple.first + " " + assetAliasMap[assetTuple.first] + " " + "\"[{\\\"aliasto\\\":\\\"" + assetTuple.second + "\\\",\\\"amount\\\":1}]\" '' ''"));
		UniValue arr = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
		string hex_str = find_value(r.get_obj(), "hex").get_str();
		
		assetSendTxVec.push_back(hex_str);
		if (count % 100 == 0)
			 printf("%.2f percentage done\n", 100.0f / (1000.0f / count));
		
	}
	printf("Sending assetsend transactions to network...\n");
	map<string, int64_t> sendTimes;
	count = 0;
	for (auto& hex_str : assetSendTxVec) {
		count++;
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
		string txid = find_value(r.get_obj(), "txid").get_str();
		sendTimes[txid] = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		if (count % 100 == 0)
			 printf("%.2f percentage done\n", 100.0f / (1000.0f / count));
		
	}
	printf("Gathering results...\n");
	float totalTime = 0;
	// wait 10 seconds	
	MilliSleep(10000);
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "tpstestinfo"));
	UniValue tpsresponse = r.get_array();
	for (int i = 0; i < tpsresponse.size(); i++) {
		UniValue responesObj = tpsresponse[i].get_obj();
		string txid = find_value(responesObj, "txid").get_str();
		int64_t timeRecv = find_value(responesObj, "time").get_int64();
		if (sendTimes.find(txid) == sendTimes.end()) {
			printf("Cannot find received txid from the sender list! skipping...\n");
			continue;
			
		}
		totalTime += timeRecv - sendTimes[txid];
	}
	totalTime /= tpsresponse.size();
	printf("totalTime %.2f, num responses %d\n", totalTime, tpsresponse.size());
}*/
BOOST_AUTO_TEST_CASE(generate_big_assetname)
{
	GenerateBlocks(5);
	printf("Running generate_big_assetname...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetnamebig", "data");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// cannot create this asset because its more than 8 chars
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew 123456789 jagassetnamebig " + gooddata + " assets 8 false 1 1 0 false ''"), runtime_error);
	// its 3 chars now so its ok
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew abc jagassetnamebig " + gooddata + " assets 8 false 1 1 0 false ''"));
}
BOOST_AUTO_TEST_CASE(generate_bad_assetmaxsupply)
{
	GenerateBlocks(5);
	printf("Running generate_bad_assetmaxsupply...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetmaxsupply", "data");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 0 max supply bad
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew abc jagassetmaxsupply " + gooddata + " assets 8 false 1 0 0 false ''"), runtime_error);
	// 1 max supply good
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew abc jagassetmaxsupply " + gooddata + " assets 8 false 1 1 0 false ''"));
	// balance > max supply
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew abc jagassetmaxsupply " + gooddata + " assets 3 false 2000 1000 0 false ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetuppercase)
{
	GenerateBlocks(5);
	printf("Running generate_assetuppercase...\n");
	UniValue r;
	AliasNew("node1", "jagassetuppercase", "data");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew upper jagassetuppercase data assets 8 false 1 1 0 false ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	string hex_str = find_value(r.get_obj(), "hex").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));

	GenerateBlocks(5);
	// assetinfo is case incensitive
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + arr[1].get_str() + " false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "symbol").get_str(), "UPPER");
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest)
{
	UniValue r;
	printf("Running generate_asset_collect_interest...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollection", "data");
	AliasNew("node1", "jagassetcollectionreceiver", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually)
	string guid = AssetNew("node1", "cad", "jagassetcollection", "data", "8", "false", "10000", "-1", "0.05");

	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionreceiver\\\",\\\"amount\\\":5000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionreceiver false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());
	// 10 hours later
	GenerateBlocks(60 * 10);
	// calc interest expect 5000 (1 + 0.05 / 60) ^ (60(10)) = ~8248
	AssetClaimInterest("node1", guid, "jagassetcollectionreceiver");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionreceiver false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 824875837095);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_checktotalsupply)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_checktotalsupply...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollectioncheck", "data");
	AliasNew("node1", "jagassetcollectioncheckreceiver", "data");
	AliasNew("node1", "jagassetcollectioncheckreceiver1", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually)
	string guid = AssetNew("node1", "cad", "jagassetcollectioncheck", "data", "8", "false", "50", "100", "0.1");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectioncheckreceiver\\\",\\\"amount\\\":20},{\\\"aliasto\\\":\\\"jagassetcollectioncheckreceiver1\\\",\\\"amount\\\":30}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectioncheckreceiver false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 20 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectioncheckreceiver1 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 30 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	UniValue totalsupply = find_value(r.get_obj(), "total_supply");
	UniValue maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), 50 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 100 * COIN);

	// 1 hour later
	GenerateBlocks(60);
	// calc interest expect 20 (1 + 0.1 / 60) ^ (60(1)) = ~22.13 and 30 (1 + 0.1 / 60) ^ (60(1)) = ~33.26
	AssetClaimInterest("node1", guid, "jagassetcollectioncheckreceiver");
	AssetClaimInterest("node1", guid, "jagassetcollectioncheckreceiver1");
	// ensure total supply and individual supplies are correct after interest claims
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectioncheckreceiver false"));
	balance = find_value(r.get_obj(), "balance");
	CAmount nBalance1 = AssetAmountFromValue(balance, 8, false);
	BOOST_CHECK_EQUAL(nBalance1, 2213841452);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectioncheckreceiver1 false"));
	balance = find_value(r.get_obj(), "balance");
	CAmount nBalance2 = AssetAmountFromValue(balance, 8, false);
	BOOST_CHECK_EQUAL(nBalance2, 3326296782);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), (nBalance1 + nBalance2));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 100 * COIN);
	CAmount supplyRemaining = 100 * COIN - (nBalance1 + nBalance2);
	// mint up to the max supply
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetupdate assets " + ValueFromAssetAmount(supplyRemaining, 8, false).write() + " 0.1 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	string hex_str = find_value(r.get_obj(), "hex").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
	GenerateBlocks(5);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), 100 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 100 * COIN);

	// totalsupply cannot go > maxsupply
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetupdate assets 0.001 0.1 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_average_balance)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_average_balance...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollectionavg", "data");
	AliasNew("node1", "jagassetcollectionrcveravg", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually)
	string guid = AssetNew("node1", "token", "jagassetcollectionavg", "data", "8", "false", "10000", "-1", "0.05");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravg false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 1000 * COIN);
	int claimheight = find_value(r.get_obj(), "height").get_int();
	// 3 hours later send 1k more
	GenerateBlocks((60 * 3) - 1);

	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":3000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravg false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 4000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	// 2 hours later send 3k more
	GenerateBlocks((60 * 2) - 1);

	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravg false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);

	// 1 hour later send 1k more
	GenerateBlocks((60 * 1) - 1);

	// total interest (1000*180 + 4000*120 + 5000*60) / 360 = 2666.67 - average balance over 6hrs, calculate interest on that balance and apply it to 5k
	// formula is  ((averagebalance*pow((1 + ((double)asset.fInterestRate / 60)), (60*6)))) - averagebalance;
	//  ((2666.67*pow((1 + (0.05 / 60)), (60*6)))) - 2666.67 = 932.5 interest (total 5932.5 balance after interest)
	AssetClaimInterest("node1", guid, "jagassetcollectionrcveravg");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravg false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 593250716124);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_update_with_average_balance)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_update_with_average_balance...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollectionavgu", "data");
	AliasNew("node1", "jagassetcollectionrcveravgu", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually), can adjust the rate
	string guid = AssetNew("node1", "mytoken", "jagassetcollectionavgu", "data", "8", "false", "10000", "-1", "0.05", "true");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravgu false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 1000 * COIN);
	int claimheight = find_value(r.get_obj(), "height").get_int();
	// 3 hours later send 1k more
	GenerateBlocks((60 * 3)-11);
	// update interest rate to 10%
	AssetUpdate("node1", guid, "pub", "''", "0.1");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":3000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravgu false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 4000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	// 2 hours later send 3k more
	GenerateBlocks((60 * 2) - 11);

	// interest rate to back to 5%
	AssetUpdate("node1", guid, "pub", "''", "0.05");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravgu false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);

	// 1 hour later send 1k more
	GenerateBlocks((60 * 1) - 11);

	// at the end set rate to 50% but this shouldn't affect the result since we set this rate recently
	AssetUpdate("node1", guid, "pub", "''", "0.5");
	// total interest (1000*180 + 4000*120 + 5000*60) / 360 = 2666.67 - average balance over 6hrs, calculate interest on that balance and apply it to 5k
	// total interest rate (0.05*180 + 0.1*120 + 0.05*60) / 360 = 0.0667% - average interest over 6hrs
	// formula is  ((averagebalance*pow((1 + ((double)asset.fInterestRate / 60)), (60*6)))) - averagebalance;
	//  ((2666.67*pow((1 + (0.0667 / 60)), (60*6)))) - 2666.67 = 1310.65 interest (total about 6310.65 balance after interest)
	AssetClaimInterest("node1", guid, "jagassetcollectionrcveravgu");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionrcveravgu false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 631064931803);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_every_block)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_every_block...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollection1", "data");
	AliasNew("node1", "jagassetcollectionreceiver1", "data");
	// setup asset with 10% interest hourly (unit test mode calculates interest hourly not annually)
	string guid = AssetNew("node1", "a", "jagassetcollection1", "data", "8", "false", "10000", "-1", "0.05");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetcollectionreceiver1\\\",\\\"amount\\\":5000}]\"", "memoassetinterest1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionreceiver1 false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());
	// 10 hours later
	// calc interest expect 5000 (1 + 0.05 / 60) ^ (60(10)) = ~8248
	for (int i = 0; i <= 60*10; i+=25) {
		AssetClaimInterest("node1", guid, "jagassetcollectionreceiver1");
		GenerateBlocks(24);
		printf("Claiming interest %d of out %d...\n", i, 60 * 10);
	}
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetcollectionreceiver1 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 824875837081);

}
BOOST_AUTO_TEST_CASE(generate_assetupdate)
{
	printf("Running generate_assetupdate...\n");
	AliasNew("node1", "jagassetupdate", "data");
	AliasNew("node2", "jagassetupdate1", "data");
	string guid = AssetNew("node1", "b", "jagassetupdate", "data");
	// update an asset that isn't yours
	UniValue r;
	//"assetupdate [asset] [public] [category=assets] [supply] [interest_rate] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetupdate " + guid + " jagassetupdate assets 1 0 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());

	AssetUpdate("node1", guid, "pub1");
	// shouldnt update data, just uses prev data because it hasnt changed
	AssetUpdate("node1", guid);
	// update supply, ensure balance gets updated properly, 5+1, 1 comes from the initial assetnew, 1 above doesn't actually get set because asset wasn't yours so total should be 6
	AssetUpdate("node1", guid, "pub12", "5");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 6*COIN);
	// update interest rate
	string guid1 = AssetNew("node1", "c", "jagassetupdate", "data", "8", "false", "1", "10", "0.1", "true");
	AssetUpdate("node1", guid1, "pub12", "''", "0.25");
	// ensure can't update interest rate (use initial asset which has can_adjust_rate set to false)
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetupdate assets 1 0.11 ''"), runtime_error);
	// can't change supply > max supply (current balance already 6, max is 10)
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetupdate assets 5 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetupdate_precision)
{
	printf("Running generate_assetupdate_precision...\n");
	UniValue r;
	for (int i = 0; i <= 8; i++) {
		string istr = boost::lexical_cast<string>(i);
		string assetName = "asset" + istr;
		string aliasName = "jagaliasprecision" + istr;
		AliasNew("node1", aliasName, "data");
		// test max supply for every possible precision
		string guid = AssetNew("node1", assetName, aliasName, "data", istr, "false", "1", "-1");
		UniValue negonevalue(UniValue::VSTR);
		negonevalue.setStr("-1");
		CAmount precisionCoin = powf(10, i);
		// get max value - 1 (1 is already the supply, and this value is cumulative)
		CAmount negonesupply = AssetAmountFromValue(negonevalue, i, false) - precisionCoin;
		string maxstr = ValueFromAssetAmount(negonesupply, i, false).get_str();
		AssetUpdate("node1", guid, "pub12", maxstr);
		// can't go above max balance (10^18) / (10^i) for i decimal places
		BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " " + aliasName + " assets 1 0 ''"), runtime_error);
		// can't create asset with more than max+1 balance or max+1 supply
		string maxstrplusone = ValueFromAssetAmount(negonesupply + (precisionCoin * 2), i, false).get_str();
		maxstr = ValueFromAssetAmount(negonesupply + precisionCoin, i, false).get_str();
		BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " false " + maxstr + " -1 0 false ''"));
		BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " false 1 " + maxstr + " 0 false ''"));
		BOOST_CHECK_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " false " + maxstrplusone + " -1 0 false ''"), runtime_error);
		BOOST_CHECK_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " false 1 " + maxstrplusone + " 0 false ''"), runtime_error);
	}
	AliasNew("node1", "badprecisionalias", "data");
	// invalid precisions
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew high badprecisionalias pub assets 9 false 1 2 0 false ''"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew low badprecisionalias pub assets -1 false 1 2 0 false ''"), runtime_error);

	// try an input range asset for 10m max with precision 0
	// for fun try to use precision 4 for input range it should default to 0
	string istr = boost::lexical_cast<string>(4);
	int i = 0;
	string assetName = "usd" + istr;
	string aliasName = "jagaliasir" + istr;
	AliasNew("node1", aliasName, "data");
	// test max supply
	string guid1 = AssetNew("node1", assetName, aliasName, "data", istr, "true", "1", "-1");
	UniValue negonevalue(UniValue::VSTR);
	negonevalue.setStr("-1");
	CAmount precisionCoin = powf(10, i);
	// get max value - 1 (1 is already the supply, and this value is cumulative)
	CAmount negonesupply = AssetAmountFromValue(negonevalue, i, true) - precisionCoin;
	string maxstr = ValueFromAssetAmount(negonesupply, i, true).get_str();
	AssetUpdate("node1", guid1, "pub12", maxstr);
	// can't go above max balance (10^18) / (10^i) for i decimal places
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid1 + " " + aliasName + " assets 1 0 ''"), runtime_error);
	// can't create asset with more than max+1 balance or max+1 supply
	string maxstrplusone = ValueFromAssetAmount(negonesupply + (precisionCoin * 2), i, true).get_str();
	maxstr = ValueFromAssetAmount(negonesupply + precisionCoin, i, true).get_str();
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " true " + maxstr + " -1 0 false ''"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " true 1 " + maxstr + " 0 false ''"));
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " true " + maxstrplusone + " -1 0 false ''"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew  " + assetName + "2 " + aliasName + " pub assets " + istr + " true 1 " + maxstrplusone + " 0 false ''"), runtime_error);

}
BOOST_AUTO_TEST_CASE(generate_assetsend)
{
	UniValue r;
	printf("Running generate_assetsend...\n");
	AliasNew("node1", "jagassetsend", "data");
	AliasNew("node2", "jagassetsend1", "data");
	string guid = AssetNew("node1", "elf", "jagassetsend", "data", "8", "false", "10", "20");
	// [{\"aliasto\":\"aliasname\",\"amount\":amount},...]
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetsend1\\\",\\\"amount\\\":7}]\"", "memoassetsend");
	// ensure amounts are correct
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	UniValue balance = find_value(r.get_obj(), "balance");
	UniValue totalsupply = find_value(r.get_obj(), "total_supply");
	UniValue maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 3 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), 10 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 20 * COIN);
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsend1 true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 7 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memoassetsend");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	// add balances
	AssetUpdate("node1", guid, "pub12", "1");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 4 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), 11 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	AssetUpdate("node1", guid, "pub12", "9");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 13 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, false), 20 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, false), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	// can't go over 20 supply
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetsend assets 1 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetsend_ranges)
{
	UniValue r;
	printf("Running generate_assetsend_ranges...\n");
	AliasNew("node1", "jagassetsendranges", "data");
	AliasNew("node2", "jagassetsendranges1", "data");
	// if use input ranges update supply and ensure adds to end of allocation, ensure balance gets updated properly
	string guid = AssetNew("node1", "msft", "jagassetsendranges", "data", "8", "true", "10", "20");
	// send range 1-2, 4-6, 8-9 and then add 1 balance and expect it to add to 10, add 9 more and expect it to add to 11, try to add one more and won't let you due to max 20 supply
	// [{\"aliasto\":\"aliasname\",\"ranges\":[{\"start\":index,\"end\":index},...]},...]
	// break ranges into 0, 3, 7
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetsendranges1\\\",\\\"ranges\\\":[{\\\"start\\\":1,\\\"end\\\":2},{\\\"start\\\":4,\\\"end\\\":6},{\\\"start\\\":8,\\\"end\\\":9}]}]\"", "memoassetsendranges");
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendranges1 true"));
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 2);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 6);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 8);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 9);
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 7);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memoassetsendranges");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	// ensure ranges are correct
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	UniValue totalsupply = find_value(r.get_obj(), "total_supply");
	UniValue maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 3);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, true), 10);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, true), 20);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	// add balances, expect to add to range 10 because total supply is 10 (0-9 range)
	AssetUpdate("node1", guid, "pub12", "1");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 4);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, true), 11);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, true), 20);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "start").get_int(), 10);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "end").get_int(), 10);
	AssetUpdate("node1", guid, "pub12", "9");
	// check balance is added to end, last range expected 10-19
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 13);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, true), 20);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, true), 20);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "start").get_int(), 10);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "end").get_int(), 19);
	// can't go over 20 supply
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate " + guid + " jagassetsendranges assets 1 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetsend_ranges2)
{
	// create an asset, assetsend the initial allocation to myself, assetallocationsend the entire allocation to 5 other aliases, 5 other aliases send all those allocations back to me, then mint 1000 new tokens
	UniValue r;
	printf("Running generate_assetsend_ranges2...\n");
	AliasNew("node1", "jagassetsendrangesowner", "data");
	AliasNew("node1", "jagassetsendrangesownerallocation", "data");
	AliasNew("node2", "jagassetsendrangesa", "data");
	AliasNew("node2", "jagassetsendrangesb", "data");
	AliasNew("node2", "jagassetsendrangesc", "data");
	AliasNew("node2", "jagassetsendrangesd", "data");
	AliasNew("node2", "jagassetsendrangese", "data");
	// if use input ranges update supply and ensure adds to end of allocation, ensure balance gets updated properly
	string guid = AssetNew("node1", "asset", "jagassetsendrangesowner", "asset", "8", "true", "1000", "1000000");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetsendrangesownerallocation\\\",\\\"ranges\\\":[{\\\"start\\\":0,\\\"end\\\":999}]}]\"", "memo1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 0);


	AssetAllocationTransfer(true, "node1", guid, "jagassetsendrangesownerallocation", "\"[{\\\"aliasto\\\":\\\"jagassetsendrangesa\\\",\\\"ranges\\\":[{\\\"start\\\":0,\\\"end\\\":199}]},{\\\"aliasto\\\":\\\"jagassetsendrangesb\\\",\\\"ranges\\\":[{\\\"start\\\":200,\\\"end\\\":399}]},{\\\"aliasto\\\":\\\"jagassetsendrangesc\\\",\\\"ranges\\\":[{\\\"start\\\":400,\\\"end\\\":599}]},{\\\"aliasto\\\":\\\"jagassetsendrangesd\\\",\\\"ranges\\\":[{\\\"start\\\":600,\\\"end\\\":799}]},{\\\"aliasto\\\":\\\"jagassetsendrangese\\\",\\\"ranges\\\":[{\\\"start\\\":800,\\\"end\\\":999}]}]\"", "memo");
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangesa true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 199);
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangesa true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 199);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangesb true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 200);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 399);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangesc true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 400);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 599);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangesd true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 600);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 799);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetsendrangese true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 800);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 999);
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 200);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");


	AssetUpdate("node1", guid, "ASSET", "1000");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " true"));
	balance = find_value(r.get_obj(), "balance");
	UniValue totalsupply = find_value(r.get_obj(), "total_supply");
	UniValue maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, true), 1000);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8, true), 2000);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8, true), 1000000);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 1000);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 1999);

}
BOOST_AUTO_TEST_CASE(generate_assettransfer)
{
	printf("Running generate_assettransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "jagasset1", "changeddata1");
	AliasNew("node2", "jagasset2", "changeddata2");
	AliasNew("node3", "jagasset3", "changeddata3");

	string guid1 = AssetNew("node1", "dow", "jagasset1", "pubdata");
	string guid2 = AssetNew("node1", "cat", "jagasset1", "pubdata");
	AssetUpdate("node1", guid1, "pub3");
	UniValue r;
	AssetTransfer("node1", "node2", guid1, "jagasset2");
	AssetTransfer("node1", "node3", guid2, "jagasset3");

	// xfer an asset that isn't yours
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assettransfer " + guid1 + " jagasset2 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());
	// update xferred asset
	AssetUpdate("node2", guid1, "public");

	// retransfer asset
	AssetTransfer("node2", "node3", guid1, "jagasset3");
}
BOOST_AUTO_TEST_CASE(generate_assetpruning)
{
	// asset's should not expire or be pruned
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_assetpruning...\n");
	AliasNew("node1", "jagprunealias1", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	string guid = AssetNew("node1", "bcf", "jagprunealias1", "pubdata");
	// we can find it as normal first
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));
	// make sure our offer alias doesn't expire
	AssetUpdate("node1", guid);
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprunealias1");
	StartNode("node2");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));

	// shouldn't be pruned
	BOOST_CHECK_NO_THROW(CallRPC("node2", "assetinfo " + guid + " false"));

	// stop node3
	StopNode("node3");
	
	AliasNew("node1", "jagprunealias1", "changeddata1");
	AssetUpdate("node1", guid);

	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetinfo " + guid + " false"));

	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));

	// try to create asset with same name
	AssetNew("node1", "bcf", "jagprunealias1", "pubdata");
	StartNode("node3");
}
BOOST_AUTO_TEST_SUITE_END ()
