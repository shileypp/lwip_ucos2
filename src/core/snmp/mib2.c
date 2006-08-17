/**
 * @file
 * [EXPERIMENTAL] Management Information Base II (RFC1213) objects and functions
 */

/*
 * Copyright (c) 2006 Axon Digital Design B.V., The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Christiaan Simons <christiaan.simons@axon.tv>
 */

#include "arch/cc.h"
#include "lwip/opt.h"
#include "lwip/snmp.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/ip.h"
#include "lwip/ip_frag.h"
#include "lwip/udp.h"
#include "lwip/snmp_asn1.h"
#include "lwip/snmp_structs.h"

#if LWIP_SNMP

/** 
 * IANA assigned enterprise ID for lwIP is 26381
 * @see http://www.iana.org/assignments/enterprise-numbers
 *
 * @note this enterprise ID is assigned to the lwIP project,
 * all object identifiers living under this ID are assigned
 * by the lwIP maintainers (contact Christiaan Simons)!
 * @note don't change this define, use snmp_set_sysobjid()
 *
 * If you need to create your own private MIB you'll need
 * to apply for your own enterprise ID with IANA:
 * http://www.iana.org/numbers.html 
 */
#define SNMP_ENTERPRISE_ID 26381
#define SNMP_SYSOBJID_LEN 7
#define SNMP_SYSOBJID {1, 3, 6, 1, 4, 1, SNMP_ENTERPRISE_ID}

#ifndef SNMP_SYSSERVICES
#define SNMP_SYSSERVICES ((1 << 6) | (1 << 3) | ((IP_FORWARD) << 2))
#endif


static void system_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void system_get_value(struct obj_def *od, u16_t len, void *value);
static void interfaces_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void interfaces_get_value(struct obj_def *od, u16_t len, void *value);
static void ifentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void ifentry_get_value(struct obj_def *od, u16_t len, void *value);
static void atentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void atentry_get_value(struct obj_def *od, u16_t len, void *value);
static void ip_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void ip_get_value(struct obj_def *od, u16_t len, void *value);
static void ip_addrentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void ip_addrentry_get_value(struct obj_def *od, u16_t len, void *value);
static void ip_rteentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void ip_rteentry_get_value(struct obj_def *od, u16_t len, void *value);
static void ip_ntomentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void ip_ntomentry_get_value(struct obj_def *od, u16_t len, void *value);
static void icmp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void icmp_get_value(struct obj_def *od, u16_t len, void *value);
#if LWIP_TCP
static void tcp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void tcp_get_value(struct obj_def *od, u16_t len, void *value);
static void tcpconnentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void tcpconnentry_get_value(struct obj_def *od, u16_t len, void *value);
#endif
static void udp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void udp_get_value(struct obj_def *od, u16_t len, void *value);
static void udpentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void udpentry_get_value(struct obj_def *od, u16_t len, void *value);
static void snmp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od);
static void snmp_get_value(struct obj_def *od, u16_t len, void *value);

/* snmp .1.3.6.1.2.1.11 */
const s32_t snmp_ids[28] = {
  1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30
};
struct mib_node* const snmp_nodes[28] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node snmp = {
  &snmp_get_object_def,
  &snmp_get_value,
  MIB_NODE_AR,
  28,
  snmp_ids,
  snmp_nodes 
};

/* dot3 and EtherLike MIB not planned. (transmission .1.3.6.1.2.1.10) */
/* historical (some say hysterical). (cmot .1.3.6.1.2.1.9) */
/* lwIP has no EGP, thus may not implement it. (egp .1.3.6.1.2.1.8) */

/* udp .1.3.6.1.2.1.7 */
const s32_t udpentry_ids[2] = { 1, 2 };
struct mib_node* const udpentry_nodes[2] = {
  NULL, NULL,
};
const struct mib_array_node udpentry = {
  &udpentry_get_object_def,
  &udpentry_get_value,
  MIB_NODE_AR,
  2,
  udpentry_ids,
  udpentry_nodes
};

const s32_t udptable_id = 1;
struct mib_node* const udptable_node = (struct mib_node* const)&udpentry;
const struct mib_array_node udptable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &udptable_id,
  &udptable_node
};

const s32_t udp_ids[5] = { 1, 2, 3, 4, 5 };
struct mib_node* const udp_nodes[5] = {
  NULL, NULL, NULL, NULL, (struct mib_node* const)&udptable
};
const struct mib_array_node udp = {
  &udp_get_object_def,
  &udp_get_value,
  MIB_NODE_AR,
  5,
  udp_ids,
  udp_nodes
};

/* tcp .1.3.6.1.2.1.6 */
#if LWIP_TCP
/* only if the TCP protocol is available may implement this group */
const s32_t tcpconnentry_ids[5] = { 1, 2, 3, 4, 5 };
struct mib_node* const tcpconnentry_nodes[5] = {
  NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node tcpconnentry = {
  &tcpconnentry_get_object_def,
  &tcpconnentry_get_value,
  MIB_NODE_AR,
  5,
  tcpconnentry_ids,
  tcpconnentry_nodes
};

const s32_t tcpconntable_id = 1;
struct mib_node* const tcpconntable_node = (struct mib_node* const)&tcpconnentry;
const struct mib_array_node tcpconntable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &tcpconntable_id,
  &tcpconntable_node
};

const s32_t tcp_ids[15] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
struct mib_node* const tcp_nodes[15] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  (struct mib_node* const)&tcpconntable, NULL, NULL
};
const struct mib_array_node tcp = {
  &tcp_get_object_def,
  &tcp_get_value,
  MIB_NODE_AR,
  15,
  tcp_ids,
  tcp_nodes
};
#endif

/* icmp .1.3.6.1.2.1.5 */
const s32_t icmp_ids[26] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
struct mib_node* const icmp_nodes[26] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node icmp = {
  &icmp_get_object_def,
  &icmp_get_value,
  MIB_NODE_AR,
  26,
  icmp_ids,
  icmp_nodes
};

const s32_t ipntomentry_ids[4] = { 1, 2, 3, 4 };
struct mib_node* const ipntomentry_nodes[4] = {
  NULL, NULL, NULL, NULL
};
const struct mib_array_node ipntomentry = {
  &ip_ntomentry_get_object_def,
  &ip_ntomentry_get_value,
  MIB_NODE_AR,
  4,
  ipntomentry_ids,
  ipntomentry_nodes
};

const s32_t ipntomtable_id = 1;
struct mib_node* const ipntomtable_node = (struct mib_node* const)&ipntomentry;
const struct mib_array_node ipntomtable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &ipntomtable_id,
  &ipntomtable_node
};

const s32_t iprteentry_ids[13] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
struct mib_node* const iprteentry_nodes[13] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node iprteentry = {
  &ip_rteentry_get_object_def,
  &ip_rteentry_get_value,
  MIB_NODE_AR,
  13,
  iprteentry_ids,
  iprteentry_nodes
};

const s32_t iprtetable_id = 1;
struct mib_node* const iprtetable_node = (struct mib_node* const)&iprteentry;
const struct mib_array_node iprtetable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &iprtetable_id,
  &iprtetable_node
};

const s32_t ipaddrentry_ids[5] = { 1, 2, 3, 4, 5 };
struct mib_node* const ipaddrentry_nodes[5] = {
  NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node ipaddrentry = {
  &ip_addrentry_get_object_def,
  &ip_addrentry_get_value,
  MIB_NODE_AR,
  5,
  ipaddrentry_ids,
  ipaddrentry_nodes
};

const s32_t ipaddrtable_id = 1;
struct mib_node* const ipaddrtable_node = (struct mib_node* const)&ipaddrentry;
const struct mib_array_node ipaddrtable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &ipaddrtable_id,
  &ipaddrtable_node
};

/* ip .1.3.6.1.2.1.4 */
const s32_t ip_ids[23] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };
struct mib_node* const ip_nodes[23] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, (struct mib_node* const)&ipaddrtable,
  (struct mib_node* const)&iprtetable, (struct mib_node* const)&ipntomtable, NULL
};
const struct mib_array_node ip = {
  &ip_get_object_def,
  &ip_get_value,
  MIB_NODE_AR,
  23,
  ip_ids,
  ip_nodes
};

const s32_t atentry_ids[3] = { 1, 2, 3 };
struct mib_node* const atentry_nodes[3] = {
  NULL, NULL, NULL
};
const struct mib_array_node atentry = {
  &atentry_get_object_def,
  &atentry_get_value,
  MIB_NODE_AR,
  3,
  atentry_ids,
  atentry_nodes
};

const s32_t attable_id = 1;
struct mib_node* const attable_node = (struct mib_node* const)&atentry;
const struct mib_array_node attable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &attable_id,
  &attable_node
};

/* at .1.3.6.1.2.1.3 */
const s32_t at_id = 1;
struct mib_node* const at_node = (struct mib_node* const)&attable;
const struct mib_array_node at = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &at_id,
  &at_node
};

const s32_t ifentry_ids[22] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
struct mib_node* const ifentry_nodes[22] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
const struct mib_array_node ifentry = {
  &ifentry_get_object_def,
  &ifentry_get_value,
  MIB_NODE_AR,
  22,
  ifentry_ids,
  ifentry_nodes
};

const s32_t iftable_id = 1;
struct mib_node* const iftable_node = (struct mib_node* const)&ifentry;
const struct mib_array_node iftable = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  &iftable_id,
  &iftable_node
};

/* interfaces .1.3.6.1.2.1.2 */
const s32_t interfaces_ids[2] = { 1, 2 };
struct mib_node* const interfaces_nodes[2] = { NULL, (struct mib_node* const)&iftable };
const struct mib_array_node interfaces = {
  &interfaces_get_object_def,
  &interfaces_get_value,
  MIB_NODE_AR,
  2,
  interfaces_ids,
  interfaces_nodes
};

/*             0 1 2 3 4 5 6 */
/* system .1.3.6.1.2.1.1 */
const s32_t sys_tem_ids[7] = { 1, 2, 3, 4, 5, 6, 7 };
struct mib_node* const sys_tem_nodes[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
/* work around name issue with 'sys_tem', some compiler(s?) seem to reserve 'system' */
const struct mib_array_node sys_tem = {
  &system_get_object_def,
  &system_get_value,
  MIB_NODE_AR,
  7,
  sys_tem_ids,
  sys_tem_nodes
};

/* mib-2 .1.3.6.1.2.1 */
#if LWIP_TCP
#define MIB2_GROUPS 8
#else
#define MIB2_GROUPS 7
#endif
const s32_t mib2_ids[MIB2_GROUPS] =
{ 
  1,
  2,
  3,
  4,
  5,
#if LWIP_TCP
  6,
#endif
  7,
  11
};
struct mib_node* const mib2_nodes[MIB2_GROUPS] = {
  (struct mib_node* const)&sys_tem,
  (struct mib_node* const)&interfaces,
  (struct mib_node* const)&at,
  (struct mib_node* const)&ip,
  (struct mib_node* const)&icmp,
#if LWIP_TCP
  (struct mib_node* const)&tcp,
#endif
  (struct mib_node* const)&udp,
  (struct mib_node* const)&snmp
};

const struct mib_array_node mib2 = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  MIB2_GROUPS,
  mib2_ids,
  mib2_nodes
};

/* mgmt .1.3.6.1.2 */
const s32_t mgmt_ids[1] = { 1 };
struct mib_node* const mgmt_nodes[1] = { (struct mib_node* const)&mib2 };
const struct mib_array_node mgmt = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  mgmt_ids,
  mgmt_nodes
};

/* internet .1.3.6.1 */
#if SNMP_PRIVATE_MIB
s32_t internet_ids[2] = { 2, 4 };
struct mib_node* const internet_nodes[2] = { (struct mib_node* const)&mgmt, (struct mib_node* const)&private };
const struct mib_array_node internet = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  2,
  internet_ids,
  internet_nodes
};
#else
const s32_t internet_ids[1] = { 2 };
struct mib_node* const internet_nodes[1] = { (struct mib_node* const)&mgmt };
const struct mib_array_node internet = {
  &noleafs_get_object_def,
  &noleafs_get_value,
  MIB_NODE_AR,
  1,
  internet_ids,
  internet_nodes
};
#endif

/** mib-2.system.sysObjectID  */
static struct snmp_obj_id sysobjid = {SNMP_SYSOBJID_LEN, SNMP_SYSOBJID};
/** enterprise ID for generic TRAPs, .iso.org.dod.internet.mgmt.mib-2.snmp */
static struct snmp_obj_id snmpgrp_id = {7,{1,3,6,1,2,1,11}};
/** mib-2.system.sysServices */
static const s32_t sysservices = SNMP_SYSSERVICES;

/** mib-2.system.sysDescr */
static const u8_t sysdescr_len_default = 4;
static const u8_t sysdescr_default[] = "lwIP";
static u8_t* sysdescr_len_ptr = (u8_t*)&sysdescr_len_default;
static u8_t* sysdescr_ptr = (u8_t*)&sysdescr_default[0];
/** mib-2.system.sysContact */
static const u8_t syscontact_len_default = 0;
static const u8_t syscontact_default[] = "";
static u8_t* syscontact_len_ptr = (u8_t*)&syscontact_len_default;
static u8_t* syscontact_ptr = (u8_t*)&syscontact_default[0];
/** mib-2.system.sysName */
static const u8_t sysname_len_default = 8;
static const u8_t sysname_default[] = "FQDN-unk";
static u8_t* sysname_len_ptr = (u8_t*)&sysname_len_default;
static u8_t* sysname_ptr = (u8_t*)&sysname_default[0];
/** mib-2.system.sysLocation */
static const u8_t syslocation_len_default = 0;
static const u8_t syslocation_default[] = "";
static u8_t* syslocation_len_ptr = (u8_t*)&syslocation_len_default;
static u8_t* syslocation_ptr = (u8_t*)&syslocation_default[0];
/** mib-2.snmp.snmpEnableAuthenTraps */
static const u8_t snmpenableauthentraps_default = 2; /* disabled */
static u8_t* snmpenableauthentraps_ptr = (u8_t*)&snmpenableauthentraps_default;

/** mib-2.interfaces.ifTable.ifEntry.ifSpecific (zeroDotZero) */
static const struct snmp_obj_id ifspecific = {2, {0, 0}};
/** mib-2.ip.ipRouteTable.ipRouteEntry.ipRouteInfo (zeroDotZero) */
static const struct snmp_obj_id iprouteinfo = {2, {0, 0}};
/** mib-2.snmp.snmpEnableAuthenTraps 1 = enabled 2 = disabled */

/* mib-2.system counter(s) */
static u32_t sysuptime = 0;

/* mib-2.ip counter(s) */
static u32_t ipinreceives = 0,
             ipinhdrerrors = 0,
             ipinaddrerrors = 0,
             ipforwdatagrams = 0,
             ipinunknownprotos = 0,
             ipindiscards = 0,
             ipindelivers = 0,
             ipoutrequests = 0,
             ipoutdiscards = 0,
             ipoutnoroutes = 0,
             ipreasmreqds = 0,
             ipreasmoks = 0,
             ipreasmfails = 0,
             ipfragoks = 0,
             ipfragfails = 0,
             ipfragcreates = 0,
             iproutingdiscards = 0;
/* mib-2.icmp counter(s) */
static u32_t icmpinmsgs = 0,
             icmpinerrors = 0,
             icmpindestunreachs = 0,
             icmpintimeexcds = 0,
             icmpinparmprobs = 0,
             icmpinsrcquenchs = 0,
             icmpinredirects = 0,
             icmpinechos = 0,
             icmpinechoreps = 0,
             icmpintimestamps = 0,
             icmpintimestampreps = 0,
             icmpinaddrmasks = 0,
             icmpinaddrmaskreps = 0,
             icmpoutmsgs = 0,
             icmpouterrors = 0,
             icmpoutdestunreachs = 0,
             icmpouttimeexcds = 0,
             icmpoutparmprobs = 0,
             icmpoutsrcquenchs = 0,
             icmpoutredirects = 0,
             icmpoutechos = 0,
             icmpoutechoreps = 0,
             icmpouttimestamps = 0,
             icmpouttimestampreps = 0,
             icmpoutaddrmasks = 0,
             icmpoutaddrmaskreps = 0;
/* mib-2.tcp counter(s) */
static u32_t tcpactiveopens = 0,
             tcppassiveopens = 0,
             tcpattemptfails = 0,
             tcpestabresets = 0,
             tcpcurrestab = 0,
             tcpinsegs = 0,
             tcpoutsegs = 0,
             tcpretranssegs = 0,
             tcpinerrs = 0,
             tcpoutrsts = 0;
/* mib-2.udp counter(s) */
static u32_t udpindatagrams = 0,
             udpnoports = 0,
             udpinerrors = 0,
             udpoutdatagrams = 0;
/* mib-2.snmp counter(s) */             
static u32_t snmpinpkts = 0,
             snmpoutpkts = 0,
             snmpinbadversions = 0,
             snmpinbadcommunitynames = 0,
             snmpinbadcommunityuses = 0,
             snmpinasnparseerrs = 0,
             snmpintoobigs = 0,
             snmpinnosuchnames = 0,
             snmpinbadvalues = 0,
             snmpinreadonlys = 0,
             snmpingenerrs = 0,
             snmpintotalreqvars = 0,
             snmpintotalsetvars = 0,
             snmpingetrequests = 0,
             snmpingetnexts = 0,
             snmpinsetrequests = 0,
             snmpingetresponses = 0,
             snmpintraps = 0,
             snmpouttoobigs = 0,
             snmpoutnosuchnames = 0,
             snmpoutbadvalues = 0,
             snmpoutgenerrs = 0,
             snmpoutgetrequests = 0,
             snmpoutgetnexts = 0,
             snmpoutsetrequests = 0,
             snmpoutgetresponses = 0,
             snmpouttraps = 0;



/* prototypes of the following functions are in lwip/src/include/lwip/snmp.h */
/**
 * Copy octet string.
 *
 * @param dst points to destination
 * @param src points to source
 * @param n number of octets to copy.
 */
void ocstrncpy(u8_t *dst, u8_t *src, u8_t n)
{
  while (n > 0)
  {
    n--;
    *dst++ = *src++;
  }
}

/**
 * Copy object identifier (s32_t) array.
 *
 * @param dst points to destination
 * @param src points to source
 * @param n number of sub identifiers to copy.
 */
void objectidncpy(s32_t *dst, s32_t *src, u8_t n)
{
  while(n > 0)
  {
    n--;
    *dst++ = *src++;
  }
}

/**
 * Initializes sysDescr pointers.
 *
 * @param str if non-NULL then copy str pointer
 * @param strlen points to string length, excluding zero terminator
 */
void snmp_set_sysdesr(u8_t *str, u8_t *strlen)
{
  if (str != NULL)
  {
    sysdescr_ptr = str;
    sysdescr_len_ptr = strlen;
  }
}

void snmp_get_sysobjid_ptr(struct snmp_obj_id **oid)
{
  *oid = &sysobjid;
}

/**
 * Initializes sysObjectID value.
 *
 * @param oid points to stuct snmp_obj_id to copy
 */
void snmp_set_sysobjid(struct snmp_obj_id *oid)
{
  sysobjid = *oid;
}

/**
 * Must be called at regular 10 msec interval from a timer interrupt
 * or signal handler depending on your runtime environment.
 */
void snmp_inc_sysuptime(void)
{
  sysuptime++;
}

void snmp_get_sysuptime(u32_t *value)
{
  *value = sysuptime;
}

/**
 * Initializes sysContact pointers,
 * e.g. ptrs to non-volatile memory external to lwIP.
 *
 * @param str if non-NULL then copy str pointer
 * @param strlen points to string length, excluding zero terminator
 */
void snmp_set_syscontact(u8_t *ocstr, u8_t *ocstrlen)
{
  if (ocstr != NULL)
  {
    syscontact_ptr = ocstr;
    syscontact_len_ptr = ocstrlen;
  }
}

/**
 * Initializes sysName pointers,
 * e.g. ptrs to non-volatile memory external to lwIP.
 *
 * @param str if non-NULL then copy str pointer
 * @param strlen points to string length, excluding zero terminator
 */
void snmp_set_sysname(u8_t *ocstr, u8_t *ocstrlen)
{
  if (ocstr != NULL)
  {
    sysname_ptr = ocstr;
    sysname_len_ptr = ocstrlen;
  }
}

/**
 * Initializes sysLocation pointers,
 * e.g. ptrs to non-volatile memory external to lwIP.
 *
 * @param str if non-NULL then copy str pointer
 * @param strlen points to string length, excluding zero terminator
 */
void snmp_set_syslocation(u8_t *ocstr, u8_t *ocstrlen)
{
  if (ocstr != NULL)
  {
    syslocation_ptr = ocstr;
    syslocation_len_ptr = ocstrlen;
  }
}


void snmp_add_ifinoctets(struct netif *ni, u32_t value)
{
  ni->ifinoctets += value;  
}

void snmp_inc_ifinucastpkts(struct netif *ni)
{
  (ni->ifinucastpkts)++;
}

void snmp_inc_ifinnucastpkts(struct netif *ni)
{
  (ni->ifinnucastpkts)++;
}

void snmp_inc_ifindiscards(struct netif *ni)
{
  (ni->ifindiscards)++;
}

void snmp_add_ifoutoctets(struct netif *ni, u32_t value)
{
  ni->ifoutoctets += value;  
}

void snmp_inc_ifoutucastpkts(struct netif *ni)
{
  (ni->ifoutucastpkts)++;
}

void snmp_inc_ifoutnucastpkts(struct netif *ni)
{
  (ni->ifoutnucastpkts)++;
}

void snmp_inc_ifoutdiscards(struct netif *ni)
{
  (ni->ifoutdiscards)++;
}

void snmp_inc_ipinreceives(void)
{
  ipinreceives++;
}

void snmp_inc_ipinhdrerrors(void)
{
  ipinhdrerrors++;
}

void snmp_inc_ipinaddrerrors(void)
{
  ipinaddrerrors++;
}

void snmp_inc_ipforwdatagrams(void)
{
  ipforwdatagrams++;
}

void snmp_inc_ipinunknownprotos(void)
{
  ipinunknownprotos++;
}

void snmp_inc_ipindiscards(void)
{
  ipindiscards++;
}

void snmp_inc_ipindelivers(void)
{
  ipindelivers++;
}

void snmp_inc_ipoutrequests(void)
{
  ipoutrequests++;
}

void snmp_inc_ipoutdiscards(void)
{
  ipoutdiscards++;
}

void snmp_inc_ipoutnoroutes(void)
{
  ipoutnoroutes++;
}

void snmp_inc_ipreasmreqds(void)
{
  ipreasmreqds++;
}

void snmp_inc_ipreasmoks(void)
{
  ipreasmoks++;
}

void snmp_inc_ipreasmfails(void)
{
  ipreasmfails++;
}

void snmp_inc_ipfragoks(void)
{
  ipfragoks++;
}

void snmp_inc_ipfragfails(void)
{
  ipfragfails++;
}

void snmp_inc_ipfragcreates(void)
{
  ipfragcreates++;
}

void snmp_inc_iproutingdiscards(void)
{
  iproutingdiscards++;
}


void snmp_inc_icmpinmsgs(void)
{
  icmpinmsgs++;
}

void snmp_inc_icmpinerrors(void)
{
  icmpinerrors++;
}

void snmp_inc_icmpindestunreachs(void)
{
  icmpindestunreachs++;
}

void snmp_inc_icmpintimeexcds(void)
{
  icmpintimeexcds++;
}

void snmp_inc_icmpinparmprobs(void)
{
  icmpinparmprobs++;
}

void snmp_inc_icmpinsrcquenchs(void)
{
  icmpinsrcquenchs++;
}

void snmp_inc_icmpinredirects(void)
{
  icmpinredirects++;
}

void snmp_inc_icmpinechos(void)
{
  icmpinechos++;
}

void snmp_inc_icmpinechoreps(void)
{ 
  icmpinechoreps++;
}

void snmp_inc_icmpintimestamps(void)
{
  icmpintimestamps++;
}

void snmp_inc_icmpintimestampreps(void)
{
  icmpintimestampreps++;
}

void snmp_inc_icmpinaddrmasks(void)
{
  icmpinaddrmasks++;
} 

void snmp_inc_icmpinaddrmaskreps(void)
{
  icmpinaddrmaskreps++;
} 

void snmp_inc_icmpoutmsgs(void)
{
  icmpoutmsgs++;
}

void snmp_inc_icmpouterrors(void)
{
  icmpouterrors++;
}

void snmp_inc_icmpoutdestunreachs(void)
{
  icmpoutdestunreachs++;
} 

void snmp_inc_icmpouttimeexcds(void)
{
  icmpouttimeexcds++;
}

void snmp_inc_icmpoutparmprobs(void)
{
  icmpoutparmprobs++;
}

void snmp_inc_icmpoutsrcquenchs(void)
{
  icmpoutsrcquenchs++;
}

void snmp_inc_icmpoutredirects(void)
{
  icmpoutredirects++;
} 

void snmp_inc_icmpoutechos(void)
{
  icmpoutechos++;
}

void snmp_inc_icmpoutechoreps(void)
{
  icmpoutechoreps++;
}

void snmp_inc_icmpouttimestamps(void)
{
  icmpouttimestamps++;
}

void snmp_inc_icmpouttimestampreps(void)
{
  icmpouttimestampreps++;
}

void snmp_inc_icmpoutaddrmasks(void)
{
  icmpoutaddrmasks++;
}

void snmp_inc_icmpoutaddrmaskreps(void)
{
  icmpoutaddrmaskreps++;
}

void snmp_inc_tcpactiveopens(void)
{
  tcpactiveopens++;
}

void snmp_inc_tcppassiveopens(void)
{
  tcppassiveopens++;
}

void snmp_inc_tcpattemptfails(void)
{
  tcpattemptfails++;
}

void snmp_inc_tcpestabresets(void)
{
  tcpestabresets++;
} 

void snmp_inc_tcpcurrestab(void)
{
  tcpcurrestab++; 
}

void snmp_inc_tcpinsegs(void)
{
  tcpinsegs++;
}

void snmp_inc_tcpoutsegs(void)
{
  tcpoutsegs++;
}

void snmp_inc_tcpretranssegs(void)
{
  tcpretranssegs++;
}

void snmp_inc_tcpinerrs(void)
{
  tcpinerrs++;
}

void snmp_inc_tcpoutrsts(void)
{
  tcpoutrsts++;
}

void snmp_inc_udpindatagrams(void)
{
  udpindatagrams++;
}

void snmp_inc_udpnoports(void)
{
  udpnoports++;
}

void snmp_inc_udpinerrors(void)
{
  udpinerrors++;
}

void snmp_inc_udpoutdatagrams(void)
{
  udpoutdatagrams++;
}

void snmp_inc_snmpinpkts(void)
{
  snmpinpkts++;
}

void snmp_inc_snmpoutpkts(void)
{
  snmpoutpkts++;
}

void snmp_inc_snmpinbadversions(void)
{
  snmpinbadversions++;
}

void snmp_inc_snmpinbadcommunitynames(void)
{
  snmpinbadcommunitynames++;
}

void snmp_inc_snmpinbadcommunityuses(void)
{
  snmpinbadcommunityuses++;
}

void snmp_inc_snmpinasnparseerrs(void)
{
  snmpinasnparseerrs++;
}

void snmp_inc_snmpintoobigs(void)
{
  snmpintoobigs++;
}

void snmp_inc_snmpinnosuchnames(void)
{
  snmpinnosuchnames++;
}

void snmp_inc_snmpinbadvalues(void)
{
  snmpinbadvalues++;
}

void snmp_inc_snmpinreadonlys(void)
{
  snmpinreadonlys++;
}

void snmp_inc_snmpingenerrs(void)
{
  snmpingenerrs++;
}

void snmp_add_snmpintotalreqvars(u8_t value)
{
  snmpintotalreqvars += value;
}

void snmp_add_snmpintotalsetvars(u8_t value)
{
  snmpintotalsetvars += value;
}

void snmp_inc_snmpingetrequests(void)
{
  snmpingetrequests++;
}

void snmp_inc_snmpingetnexts(void)
{
  snmpingetnexts++;
}

void snmp_inc_snmpinsetrequests(void)
{
  snmpinsetrequests++;
}

void snmp_inc_snmpingetresponses(void)
{
  snmpingetresponses++;
}

void snmp_inc_snmpintraps(void)
{
  snmpintraps++;
}

void snmp_inc_snmpouttoobigs(void)
{
  snmpouttoobigs++;
}

void snmp_inc_snmpoutnosuchnames(void)
{
  snmpoutnosuchnames++;
}

void snmp_inc_snmpoutbadvalues(void)
{
  snmpoutbadvalues++;
}

void snmp_inc_snmpoutgenerrs(void)
{
  snmpoutgenerrs++;
}

void snmp_inc_snmpoutgetrequests(void)
{
  snmpoutgetrequests++;
}

void snmp_inc_snmpoutgetnexts(void)
{
  snmpoutgetnexts++;
}

void snmp_inc_snmpoutsetrequests(void)
{
  snmpoutsetrequests++;
}

void snmp_inc_snmpoutgetresponses(void)
{
  snmpoutgetresponses++;
}

void snmp_inc_snmpouttraps(void)
{
  snmpouttraps++;
}

void snmp_get_snmpgrpid_ptr(struct snmp_obj_id **oid)
{
  *oid = &snmpgrp_id;
}

void snmp_set_snmpenableauthentraps(u8_t *value)
{
  if (value != NULL)
  {
    snmpenableauthentraps_ptr = value;
  }
}

void
noleafs_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if (ident_len){}
  if (ident){}
  od->instance = MIB_OBJECT_NONE;
}

void                                                  
noleafs_get_value(struct obj_def *od, u16_t len, void *value)
{
  if (od){}
  if (len){}
  if (value){}
}




/**
 * Returns systems object definitions.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.0 (object id trailer)
 * @param od points to object definition.
 */
static void
system_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  u8_t id;

  if ((ident_len == 2) && (ident[1] == 0))
  { 
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;
    
    id = ident[0];
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("get_object_def system.%"U16_F".0",(u16_t)id));
    switch (id)
    {
      case 1: /* sysDescr */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
        od->v_len = *sysdescr_len_ptr;
        break;
      case 2: /* sysObjectID */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OBJ_ID);
        od->v_len = SNMP_SYSOBJID_LEN * sizeof(s32_t);
        break;
      case 3: /* sysUpTime */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_TIMETICKS);
        od->v_len = sizeof(u32_t);
        break;
      case 4: /* sysContact */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
        od->v_len = *syscontact_len_ptr;
        break;
      case 5: /* sysName */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
        od->v_len = *sysname_len_ptr;
        break;
      case 6: /* sysLocation */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
        od->v_len = *syslocation_len_ptr;
        break;
      case 7: /* sysServices */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      default:
        LWIP_DEBUGF(SNMP_MIB_DEBUG,("system_get_object_def: no such object"));
        od->instance = MIB_OBJECT_NONE;
        break;
    };
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("system_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

/**
 * Returns system object value.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.0 (object id trailer)
 * @param len return value space (in bytes)
 * @param value points to (varbind) space to copy value into.
 */
static void                                                  
system_get_value(struct obj_def *od, u16_t len, void *value)
{
  u8_t id;

  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* sysDescr */
      ocstrncpy(value,sysdescr_ptr,len);
      break;
    case 2: /* sysObjectID */
      objectidncpy((s32_t*)value,(s32_t*)sysobjid.id,len / sizeof(s32_t));
      break;
    case 3: /* sysUpTime */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = sysuptime;
      }
      break;
    case 4: /* sysContact */
      ocstrncpy(value,syscontact_ptr,len);
      break;
    case 5: /* sysName */
      ocstrncpy(value,sysname_ptr,len);
      break;
    case 6: /* sysLocation */
      ocstrncpy(value,syslocation_ptr,len);
      break;
    case 7: /* sysServices */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = sysservices;
      }
      break;
  };
}

/**
 * Returns interfaces.ifnumber object definition.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.index
 * @param od points to object definition.
 */
static void
interfaces_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 2) && (ident[0] == 1) && (ident[1] == 0))
  {
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;

    od->instance = MIB_OBJECT_SCALAR;
    od->access = MIB_OBJECT_READ_ONLY;
    od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
    od->v_len = sizeof(s32_t);
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("interfaces_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

/**
 * Returns interfaces.ifnumber object value.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.0 (object id trailer)
 * @param len return value space (in bytes)
 * @param value points to (varbind) space to copy value into.
 */
static void                                                  
interfaces_get_value(struct obj_def *od, u16_t len, void *value)
{
  if (len){}
  if (od->id_inst_ptr[0] == 1)
  {
    s32_t *sint_ptr = value;
    *sint_ptr = netif_cnt;
  }
}

/**
 * Returns ifentry object definitions.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.index
 * @param od points to object definition.
 */
static void
ifentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  u8_t id;

  if ((ident_len == 2) && (ident[1] > 0) && (ident[1] <= netif_cnt))
  {
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;
    
    id = ident[0];
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("get_object_def ifentry.%"U16_F".",(u16_t)id));
    switch (id)
    {
      case 1: /* ifIndex */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      case 2: /* ifDescr */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
        /** @todo this should be some sort of sizeof(struct netif.name) */
        od->v_len = 2;
        break;
      case 3: /* ifType */
      case 4: /* ifMtu */
      case 8: /* ifOperStatus */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      case 5: /* ifSpeed */
      case 21: /* ifOutQLen */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_GAUGE);
        od->v_len = sizeof(u32_t);
        break;
      case 6: /* ifPhysAddress */
        {
          struct netif *netif = netif_list;
          u16_t i, ifidx;

          ifidx = ident[1] - 1;
          i = 0;
          while ((netif != NULL) && (i < ifidx))
          {
            netif = netif->next;
            i++;
          }
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_ONLY;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
          od->v_len = netif->hwaddr_len;
        }
        break;
      case 7: /* ifAdminStatus */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      case 9: /* ifLastChange */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_TIMETICKS);
        od->v_len = sizeof(u32_t);
        break;
      case 10: /* ifInOctets */
      case 11: /* ifInUcastPkts */
      case 12: /* ifInNUcastPkts */
      case 13: /* ifInDiscarts */
      case 14: /* ifInErrors */
      case 15: /* ifInUnkownProtos */
      case 16: /* ifOutOctets */
      case 17: /* ifOutUcastPkts */
      case 18: /* ifOutNUcastPkts */
      case 19: /* ifOutDiscarts */
      case 20: /* ifOutErrors */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_COUNTER);
        od->v_len = sizeof(u32_t);
        break;
      case 22: /* ifSpecific */
        /** @note returning zeroDotZero (0.0) no media specific MIB support */
        od->instance = MIB_OBJECT_TAB;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OBJ_ID);
        od->v_len = ifspecific.len * sizeof(s32_t);
        break;
      default:
        LWIP_DEBUGF(SNMP_MIB_DEBUG,("ifentry_get_object_def: no such object"));
        od->instance = MIB_OBJECT_NONE;
        break;
    };
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("ifentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

/**
 * Returns ifentry object value.
 *
 * @param ident_len the address length (2)
 * @param ident points to objectname.0 (object id trailer)
 * @param len return value space (in bytes)
 * @param value points to (varbind) space to copy value into.
 */
static void                                                  
ifentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  struct netif *netif = netif_list;
  u16_t i, ifidx;
  u8_t id;

  ifidx = od->id_inst_ptr[1] - 1;
  i = 0;
  while ((netif != NULL) && (i < ifidx))
  {
    netif = netif->next;
    i++;
  }
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* ifIndex */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = od->id_inst_ptr[1];
      }
      break;
    case 2: /* ifDescr */
      ocstrncpy(value,(u8_t*)netif->name,len);
      break;
    case 3: /* ifType */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = netif->link_type;
      }
      break;
    case 4: /* ifMtu */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = netif->mtu;
      }
      break;
    case 5: /* ifSpeed */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->link_speed;
      }
      break;
    case 6: /* ifPhysAddress */
      ocstrncpy(value,netif->hwaddr,len);
      break;
    case 7: /* ifAdminStatus */
    case 8: /* ifOperStatus */
      {
        s32_t *sint_ptr = value;
        if (netif_is_up(netif))
        {
          *sint_ptr = 1;
        }
        else
        {
          *sint_ptr = 2;
        }
      }
      break;
    case 9: /* ifLastChange */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ts;
      }
      break;
    case 10: /* ifInOctets */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifinoctets;
      }
      break;
    case 11: /* ifInUcastPkts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifinucastpkts;
      }
      break;
    case 12: /* ifInNUcastPkts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifinnucastpkts;
      }
      break;
    case 13: /* ifInDiscarts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifindiscards;
      }
      break;
    case 14: /* ifInErrors */
    case 15: /* ifInUnkownProtos */
      /** @todo add these counters! */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = 0;
      }
      break;
    case 16: /* ifOutOctets */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifoutoctets;
      }
      break;
    case 17: /* ifOutUcastPkts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifoutucastpkts;
      }
      break;
    case 18: /* ifOutNUcastPkts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifoutnucastpkts;
      }
      break;
    case 19: /* ifOutDiscarts */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = netif->ifoutdiscards;
      }
      break;
    case 20: /* ifOutErrors */
       /** @todo add this counter! */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = 0;
      }
      break;
    case 21: /* ifOutQLen */
      /** @todo figure out if this must be 0 (no queue) or 1? */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = 0;
      }
      break;
    case 22: /* ifSpecific */
      objectidncpy((s32_t*)value,(s32_t*)ifspecific.id,len / sizeof(s32_t));
      break;
  };
}

/**
 * Returns atentry object definitions.
 *
 * @param ident_len the address length (6)
 * @param ident points to objectname.atifindex.atnetaddress
 * @param od points to object definition.
 *
 * @todo std says objects are writeable, can we ignore it?
 */
static void
atentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 6) && 
      (ident[1] > 0) && (ident[1] <= netif_cnt))
  {
    struct eth_addr* ethaddr_ret;
    struct ip_addr* ipaddr_ret;
    struct ip_addr ip;
    struct netif *netif = netif_list;
    u16_t i, ifidx;

    ifidx = ident[1] - 1;
    i = 0;
    while ((netif != NULL) && (i < ifidx))
    {
      netif = netif->next;
      i++;
    }
    ip.addr = ident[2];
    ip.addr <<= 8;
    ip.addr |= ident[3];
    ip.addr <<= 8;
    ip.addr |= ident[4];
    ip.addr <<= 8;
    ip.addr |= ident[5];
    ip.addr = htonl(ip.addr);
    
    if (etharp_find_addr(netif, &ip, &ethaddr_ret, &ipaddr_ret) > -1)
    {
      od->id_inst_len = ident_len;
      od->id_inst_ptr = ident;

      switch (ident[0])
      {
        case 1: /* atIfIndex */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          od->addr = NULL;
          break;
        case 2: /* atPhysAddress */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
          od->v_len = sizeof(struct eth_addr);
          od->addr = ethaddr_ret;
          break;
        case 3: /* atNetAddress */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_IPADDR);
          od->v_len = 4;
          od->addr = ipaddr_ret;
          break;
        default:
          LWIP_DEBUGF(SNMP_MIB_DEBUG,("atentry_get_object_def: no such object"));
          od->instance = MIB_OBJECT_NONE;
          break;
      }
    }
    else
    {
      LWIP_DEBUGF(SNMP_MIB_DEBUG,("atentry_get_object_def: no scalar"));
      od->instance = MIB_OBJECT_NONE;
    }
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("atentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
atentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  u8_t id;

  if (len) {}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* atIfIndex */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = od->id_inst_ptr[1];
      }
      break;
    case 2: /* atPhysAddress */
      {
        struct eth_addr *dst = value;
        struct eth_addr *src = od->addr;

        *dst = *src;
      }
      break;
    case 3: /* atNetAddress */
      {
        struct ip_addr *dst = value;
        struct ip_addr *src = od->addr;

        *dst = *src;
      }
      break;
  }
}


static void
ip_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  u8_t id;

  if ((ident_len == 2) && (ident[1] == 0))
  { 
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;
    
    id = ident[0];
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("get_object_def ip.%"U16_F".0",(u16_t)id));
    switch (id)
    {
      case 1: /* ipForwarding */
      case 2: /* ipDefaultTTL */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      case 3: /* ipInReceives */
      case 4: /* ipInHdrErrors */
      case 5: /* ipInAddrErrors */
      case 6: /* ipForwDatagrams */
      case 7: /* ipInUnknownProtos */
      case 8: /* ipInDiscards */
      case 9: /* ipInDelivers */
      case 10: /* ipOutRequests */
      case 11: /* ipOutDiscards */
      case 12: /* ipOutNoRoutes */
      case 14: /* ipReasmReqds */
      case 15: /* ipReasmOKs */
      case 16: /* ipReasmFails */
      case 17: /* ipFragOKs */
      case 18: /* ipFragFails */
      case 19: /* ipFragCreates */
      case 23: /* ipRoutingDiscards */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_COUNTER);
        od->v_len = sizeof(u32_t);
        break;
      case 13: /* ipReasmTimeout */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      default:
        LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_get_object_def: no such object"));
        od->instance = MIB_OBJECT_NONE;
        break;
    };
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

/** @todo ipForwarding is writeable, but we may return badValue,
          in lwIP this is a compile-time switch
          we will also return a badvalue for wring a default TTL
          which differs from our compile time default */
static void
ip_get_value(struct obj_def *od, u16_t len, void *value)
{
  u8_t id;

  if (len) {}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* ipForwarding */
      {
        s32_t *sint_ptr = value;
#if IP_FORWARD
        /* forwarding */
        *sint_ptr = 1;
#else
        /* not-forwarding */
        *sint_ptr = 2;
#endif
      }
      break;
    case 2: /* ipDefaultTTL */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = IP_DEFAULT_TTL;
      }
      break;
    case 3: /* ipInReceives */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipinreceives;
      }
      break;
    case 4: /* ipInHdrErrors */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipinhdrerrors;
      }
      break;
    case 5: /* ipInAddrErrors */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipinaddrerrors;
      }
      break;
    case 6: /* ipForwDatagrams */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipforwdatagrams;
      }
      break;
    case 7: /* ipInUnknownProtos */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipinunknownprotos;
      }
      break;
    case 8: /* ipInDiscards */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipindiscards;
      }
      break;
    case 9: /* ipInDelivers */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipindelivers;
      }
      break;
    case 10: /* ipOutRequests */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipoutrequests;
      }
      break;
    case 11: /* ipOutDiscards */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipoutdiscards;
      }
      break;
    case 12: /* ipOutNoRoutes */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipoutnoroutes;
      }
      break;
    case 13: /* ipReasmTimeout */
      {
        s32_t *sint_ptr = value;
#if IP_REASSEMBLY
        *sint_ptr = IP_REASS_MAXAGE;
#else
        *sint_ptr = 0;
#endif
      }
      break;
    case 14: /* ipReasmReqds */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipreasmreqds;
      }
      break;
    case 15: /* ipReasmOKs */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipreasmoks;
      }
      break;
    case 16: /* ipReasmFails */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipreasmfails;
      }
      break;
    case 17: /* ipFragOKs */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipfragoks;
      }
      break;
    case 18: /* ipFragFails */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipfragfails;
      }
      break;
    case 19: /* ipFragCreates */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = ipfragcreates;
      }
      break;
    case 23: /* ipRoutingDiscards */
      /** @todo can lwIP discard routes at all?? hardwire this to 0?? */
      {
        u32_t *uint_ptr = value;
        *uint_ptr = iproutingdiscards;
      }
      break;
  };
}

static void
ip_addrentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if (ident_len == 5)
  {
    struct ip_addr ip;
    struct netif *netif = netif_list;

    ip.addr = ident[1];
    ip.addr <<= 8;
    ip.addr |= ident[2];
    ip.addr <<= 8;
    ip.addr |= ident[3];
    ip.addr <<= 8;
    ip.addr |= ident[4];
    ip.addr = htonl(ip.addr);

    while ((netif != NULL) && !ip_addr_cmp(&ip, &netif->ip_addr))
    {
      netif = netif->next;
    }
    
    if (netif != NULL)
    {
      u8_t id;

      od->id_inst_len = ident_len;
      od->id_inst_ptr = ident;
      od->addr = netif;

      id = ident[0];
      switch (id)
      {
        case 1: /* ipAdEntAddr */
        case 3: /* ipAdEntNetMask */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_ONLY;
          od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_IPADDR);
          od->v_len = 4;
          break;
        case 2: /* ipAdEntIfIndex */
        case 4: /* ipAdEntBcastAddr */
        case 5: /* ipAdEntReasmMaxSize */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_ONLY;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          break;
        default:
          LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_addrentry_get_object_def: no such object"));
          od->instance = MIB_OBJECT_NONE;
          break;
      }
    }
    else
    {
      LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_addrentry_get_object_def: no scalar"));
      od->instance = MIB_OBJECT_NONE;
    }
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_addrentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
ip_addrentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  u8_t id;

  if (len) {}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* ipAdEntAddr */
      {
        struct ip_addr *dst = value;
        struct netif *netif;
        struct ip_addr *src;

        netif = od->addr;
        src = &netif->ip_addr;
        *dst = *src;
      }
      break;
    case 2: /* ipAdEntIfIndex */
      {
        s32_t *sint_ptr = value;
        struct netif *netif = netif_list;
        u16_t i;

        i = 0;
        while ((netif != NULL) && (netif != od->addr))
        {
          netif = netif->next;
          i++;
        }
        *sint_ptr = i + 1;
      }
      break;
    case 3: /* ipAdEntNetMask */
      {
        struct ip_addr *dst = value;
        struct netif *netif;
        struct ip_addr *src;

        netif = od->addr;
        src = &netif->netmask;
        *dst = *src;
      }
      break;
    case 4: /* ipAdEntBcastAddr */
      {
        s32_t *sint_ptr = value;      

        /* lwIP oddity, there's no broadcast
          address in the netif we can rely on */
        *sint_ptr = ip_addr_broadcast.addr & 1;
      }
      break;
    case 5: /* ipAdEntReasmMaxSize */
      {
        s32_t *sint_ptr = value;      
#if IP_REASSEMBLY
        *sint_ptr = (IP_HLEN + IP_REASS_BUFSIZE);
#else
        /** @todo returning MTU would be a bad thing and
           returning a wild guess like '576' isn't good either */
        *sint_ptr = 0;
#endif
      }
      break;
  }
}

/** 
 * @note
 * lwIP IP routing is currently using the network addresses in netif_list.
 * if no suitable network IP is found in netif_list, the default_netif is used.
 */
static void
ip_rteentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  u8_t id;

  if (ident_len == 5)
  {
    struct ip_addr dest;
    struct netif *netif;

    dest.addr = ident[1];
    dest.addr <<= 8;
    dest.addr |= ident[2];
    dest.addr <<= 8;
    dest.addr |= ident[3];
    dest.addr <<= 8;
    dest.addr |= ident[4];
    dest.addr = htonl(dest.addr);

    if (dest.addr == 0)
    {
      /* ip_route() uses default netif for default route */
      netif = netif_default;
    }
    else
    {
      /* not using ip_route(), need exact match! */
      netif = netif_list;
      while ((netif != NULL) && 
              !ip_addr_netcmp(&dest, &(netif->ip_addr), &(netif->netmask)) )
      {      
        netif = netif->next;
      }
    }    
    if (netif != NULL)
    {
      od->id_inst_len = ident_len;
      od->id_inst_ptr = ident;
      od->addr = netif;

      id = ident[0];
      switch (id)
      {
        case 1: /* ipRouteDest */
        case 7: /* ipRouteNextHop */
        case 11: /* ipRouteMask */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_IPADDR);
          od->v_len = 4;
          break;
        case 2: /* ipRouteIfIndex */
        case 3: /* ipRouteMetric1 */
        case 4: /* ipRouteMetric2 */
        case 5: /* ipRouteMetric3 */
        case 6: /* ipRouteMetric4 */
        case 8: /* ipRouteType */
        case 10: /* ipRouteAge */
        case 12: /* ipRouteMetric5 */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          break;
        case 9: /* ipRouteProto */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_ONLY;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          break;
        case 13: /* ipRouteInfo */
          /** @note returning zeroDotZero (0.0) no routing protocol specific MIB */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_ONLY;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OBJ_ID);
          od->v_len = iprouteinfo.len * sizeof(s32_t);
          break;
        default:
          LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_rteentry_get_object_def: no such object"));
          od->instance = MIB_OBJECT_NONE;
          break;
      }
    }
    else
    {
      LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_rteentry_get_object_def: no scalar"));
      od->instance = MIB_OBJECT_NONE;
    }
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_rteentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
ip_rteentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  struct netif *netif;
  struct ip_addr dest;
  s32_t *ident;
  u8_t id;

  netif = od->addr;
  ident = od->id_inst_ptr;
  dest.addr = ident[1];
  dest.addr <<= 8;
  dest.addr |= ident[2];
  dest.addr <<= 8;
  dest.addr |= ident[3];
  dest.addr <<= 8;
  dest.addr |= ident[4];
  dest.addr = htonl(dest.addr);

  id = ident[0];
  switch (id)
  {
    case 1: /* ipRouteDest */
      {
        struct ip_addr *dst = value;

        if (dest.addr == 0)
        {
          /* default rte has 0.0.0.0 dest */
          dst->addr = 0;
        }
        else
        {
          /* netifs have netaddress dest */
          dst->addr = netif->ip_addr.addr & netif->netmask.addr;
        }
      }
      break;
    case 2: /* ipRouteIfIndex */
      {
        struct netif *ni = netif_list;
        s32_t *sint_ptr = value;
        u16_t i;

        i = 0;
        while ((ni != NULL) && (ni != netif))
        {
          ni = ni->next;
          i++;
        }
        *sint_ptr = i + 1;
      }
      break;
    case 3: /* ipRouteMetric1 */         
      {
        s32_t *sint_ptr = value;

        if (dest.addr == 0)
        {
          /* default rte has metric 1 */
          *sint_ptr = 1;
        }
        else
        {
          /* other rtes have metric 0 */
          *sint_ptr = 0;
        }
      }
      break;
    case 4: /* ipRouteMetric2 */
    case 5: /* ipRouteMetric3 */
    case 6: /* ipRouteMetric4 */
    case 12: /* ipRouteMetric5 */
      {
        s32_t *sint_ptr = value;
        /* not used */
        *sint_ptr = -1;
      }
      break;
    case 7: /* ipRouteNextHop */
      {
        struct ip_addr *dst = value;

        if (dest.addr == 0)
        {
          /* default rte: gateway */
          *dst = netif->gw;
        }
        else
        {
          /* other rtes: netif ip_addr  */
          *dst = netif->ip_addr;
        }
      }
      break;
    case 8: /* ipRouteType */
      {
        s32_t *sint_ptr = value;

        if (dest.addr == 0)
        {
          /* default rte is indirect */
          *sint_ptr = 4;
        }
        else
        {
          /* other rtes are direct */
          *sint_ptr = 3;
        }
      }
      break;
    case 9: /* ipRouteProto */
      {
        s32_t *sint_ptr = value;
        /* locally defined routes */
        *sint_ptr = 2;
      }
      break;
    case 10: /* ipRouteAge */
      {
        s32_t *sint_ptr = value;
        /** @todo (sysuptime - timestamp last change) / 100 */
        *sint_ptr = 0;
      }
      break;
    case 11: /* ipRouteMask */
      {
        struct ip_addr *dst = value;

        if (dest.addr == 0)
        {
          /* default rte use 0.0.0.0 mask */
          dst->addr = 0;
        }
        else
        {
          /* other rtes use netmask */
          *dst = netif->netmask;
        }
      }
      break;
    case 13: /* ipRouteInfo */
      objectidncpy((s32_t*)value,(s32_t*)iprouteinfo.id,len / sizeof(s32_t));
      break;
  }
}

static void
ip_ntomentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 6) && 
      (ident[1] > 0) && (ident[1] <= netif_cnt))
  {
    struct eth_addr* ethaddr_ret;
    struct ip_addr* ipaddr_ret;
    struct ip_addr ip;
    struct netif *netif = netif_list;
    u16_t i, ifidx;

    ifidx = ident[1] - 1;
    i = 0;
    while ((netif != NULL) && (i < ifidx))
    {
      netif = netif->next;
      i++;
    }
    ip.addr = ident[2];
    ip.addr <<= 8;
    ip.addr |= ident[3];
    ip.addr <<= 8;
    ip.addr |= ident[4];
    ip.addr <<= 8;
    ip.addr |= ident[5];
    ip.addr = htonl(ip.addr);
    
    if (etharp_find_addr(netif, &ip, &ethaddr_ret, &ipaddr_ret) > -1)
    {
      u8_t id;

      od->id_inst_len = ident_len;
      od->id_inst_ptr = ident;

      id = ident[0];
      switch (id)
      {
        case 1: /* ipNetToMediaIfIndex */
        case 4: /* ipNetToMediaType */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          od->addr = NULL;
          break;
        case 2: /* ipNetToMediaPhysAddress */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_OC_STR);
          od->v_len = sizeof(struct eth_addr);
          od->addr = ethaddr_ret;
          break;
        case 3: /* ipNetToMediaNetAddress */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_IPADDR);
          od->v_len = 4;
          od->addr = ipaddr_ret;
          break;
        default:
          LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_ntomentry_get_object_def: no such object"));
          od->instance = MIB_OBJECT_NONE;
          break;
      }
    }
    else
    {
      LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_ntomentry_get_object_def: no scalar"));
      od->instance = MIB_OBJECT_NONE;
    }
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("ip_ntomentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
ip_ntomentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  u8_t id;

  if (len){}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* ipNetToMediaIfIndex */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = od->id_inst_ptr[1];
      }
      break;
    case 2: /* ipNetToMediaPhysAddress */
      {
        struct eth_addr *dst = value;
        struct eth_addr *src = od->addr;

        *dst = *src;
      }
      break;
    case 3: /* ipNetToMediaNetAddress */
      {
        struct ip_addr *dst = value;
        struct ip_addr *src = od->addr;

        *dst = *src;
      }
      break;
    case 4: /* ipNetToMediaType */
      {
        s32_t *sint_ptr = value;
        /* dynamic (?) */
        *sint_ptr = 3;
      }
      break;
  }
}

static void
icmp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 2) && (ident[1] == 0) &&
      (ident[0] > 0) && (ident[0] < 27))
  {
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;

    od->instance = MIB_OBJECT_SCALAR;
    od->access = MIB_OBJECT_READ_ONLY;
    od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_COUNTER);
    od->v_len = sizeof(u32_t);
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("icmp_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
icmp_get_value(struct obj_def *od, u16_t len, void *value)
{
  u32_t *uint_ptr = value;
  u8_t id;

  if (len){}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* icmpInMsgs */
      *uint_ptr = icmpinmsgs;
      break;
    case 2: /* icmpInErrors */
      *uint_ptr = icmpinerrors;
      break;
    case 3: /* icmpInDestUnreachs */
      *uint_ptr = icmpindestunreachs;
      break;
    case 4: /* icmpInTimeExcds */
      *uint_ptr = icmpintimeexcds;
      break;
    case 5: /* icmpInParmProbs */
      *uint_ptr = icmpinparmprobs;
      break;
    case 6: /* icmpInSrcQuenchs */
      *uint_ptr = icmpinsrcquenchs;
      break;
    case 7: /* icmpInRedirects */
      *uint_ptr = icmpinredirects;
      break;
    case 8: /* icmpInEchos */
      *uint_ptr = icmpinechos;
      break;
    case 9: /* icmpInEchoReps */
      *uint_ptr = icmpinechoreps;
      break;
    case 10: /* icmpInTimestamps */
      *uint_ptr = icmpintimestamps;
      break;
    case 11: /* icmpInTimestampReps */
      *uint_ptr = icmpintimestampreps;
      break;
    case 12: /* icmpInAddrMasks */
      *uint_ptr = icmpinaddrmasks;
      break;
    case 13: /* icmpInAddrMaskReps */
      *uint_ptr = icmpinaddrmaskreps;
      break;
    case 14: /* icmpOutMsgs */
      *uint_ptr = icmpoutmsgs; 
      break;
    case 15: /* icmpOutErrors */
      *uint_ptr = icmpouterrors;
      break;
    case 16: /* icmpOutDestUnreachs */
      *uint_ptr = icmpoutdestunreachs;
      break;
    case 17: /* icmpOutTimeExcds */
      *uint_ptr = icmpouttimeexcds;
      break;
    case 18: /* icmpOutParmProbs */
      *uint_ptr = icmpoutparmprobs;
      break;
    case 19: /* icmpOutSrcQuenchs */
      *uint_ptr = icmpoutsrcquenchs;
      break;
    case 20: /* icmpOutRedirects */
      *uint_ptr = icmpoutredirects;
      break;
    case 21: /* icmpOutEchos */
      *uint_ptr = icmpoutechos;
      break;
    case 22: /* icmpOutEchoReps */
      *uint_ptr = icmpoutechoreps;
      break;
    case 23: /* icmpOutTimestamps */
      *uint_ptr = icmpouttimestamps;
      break;
    case 24: /* icmpOutTimestampReps */
      *uint_ptr = icmpouttimestampreps;
      break;
    case 25: /* icmpOutAddrMasks */
      *uint_ptr = icmpoutaddrmasks;
      break;
    case 26: /* icmpOutAddrMaskReps */
      *uint_ptr = icmpoutaddrmaskreps;
      break;
  }
}

#if LWIP_TCP
/** @todo tcp grp */
static void
tcp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
}

static void
tcp_get_value(struct obj_def *od, u16_t len, void *value)
{
}

static void
tcpconnentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
}

static void
tcpconnentry_get_value(struct obj_def *od, u16_t len, void *value)
{
}
#endif

static void
udp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 2) && (ident[1] == 0) &&
      (ident[0] > 0) && (ident[0] < 6))
  {
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;

    od->instance = MIB_OBJECT_SCALAR;
    od->access = MIB_OBJECT_READ_ONLY;
    od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_COUNTER);
    od->v_len = sizeof(u32_t);
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("udp_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
udp_get_value(struct obj_def *od, u16_t len, void *value)
{
  u32_t *uint_ptr = value;
  u8_t id;

  if (len){}
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* udpInDatagrams */
      *uint_ptr = udpindatagrams;
      break;
    case 2: /* udpNoPorts */
      *uint_ptr = udpnoports;
      break;
    case 3: /* udpInErrors */
      *uint_ptr = udpinerrors;
      break;
    case 4: /* udpOutDatagrams */
      *uint_ptr = udpoutdatagrams;
      break;
  }
}

static void
udpentry_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if (ident_len == 6)
  {
    struct udp_pcb *pcb;
    struct ip_addr ip;
    u16_t port;

    ip.addr = ident[1];
    ip.addr <<= 8;
    ip.addr |= ident[2];
    ip.addr <<= 8;
    ip.addr |= ident[3];
    ip.addr <<= 8;
    ip.addr |= ident[4];
    ip.addr = htonl(ip.addr);

    port = ident[5];

    pcb = udp_pcbs;
    while ((pcb != NULL) &&
            (pcb->local_ip.addr != ip.addr) &&
            (pcb->local_port != port))
    {      
      pcb = pcb->next;
    }
    
    if (pcb != NULL)
    {
      od->id_inst_len = ident_len;
      od->id_inst_ptr = ident;
      od->addr = pcb;

      switch (ident[0])
      {
        case 1: /* udpLocalAddress */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_IPADDR);
          od->v_len = 4;
          break;
        case 2: /* udpLocalPort */
          od->instance = MIB_OBJECT_TAB;
          od->access = MIB_OBJECT_READ_WRITE;
          od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
          od->v_len = sizeof(s32_t);
          break;
        default:
          LWIP_DEBUGF(SNMP_MIB_DEBUG,("udpentry_get_object_def: no such object"));
          od->instance = MIB_OBJECT_NONE;
          break;
      }
    }
    else
    {
      LWIP_DEBUGF(SNMP_MIB_DEBUG,("udpentry_get_object_def: no scalar"));
      od->instance = MIB_OBJECT_NONE;
    }
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("udpentry_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
udpentry_get_value(struct obj_def *od, u16_t len, void *value)
{
  struct udp_pcb *pcb;
  u8_t id;

  if (len){}
  pcb = od->addr;
  id = od->id_inst_ptr[0];
  switch (id)
  {
    case 1: /* udpLocalAddress */
      {
        struct ip_addr *dst = value;
        struct ip_addr *src = &pcb->local_ip;
        *dst = *src;
      }
      break;
    case 2: /* udpLocalPort */
      {
        s32_t *sint_ptr = value;
        *sint_ptr = pcb->local_port;
      }
      break;
  }
}

static void
snmp_get_object_def(u8_t ident_len, s32_t *ident, struct obj_def *od)
{
  if ((ident_len == 2) && (ident[1] == 0))
  {
    u8_t id;

    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;

    id = ident[0];
    switch (id)
    {
      case 1: /* snmpInPkts */
      case 2: /* snmpOutPkts */
      case 3: /* snmpInBadVersions */
      case 4: /* snmpInBadCommunityNames */
      case 5: /* snmpInBadCommunityUses */
      case 6: /* snmpInASNParseErrs */
      case 8: /* snmpInTooBigs */
      case 9: /* snmpInNoSuchNames */
      case 10: /* snmpInBadValues */
      case 11: /* snmpInReadOnlys */
      case 12: /* snmpInGenErrs */
      case 13: /* snmpInTotalReqVars */
      case 14: /* snmpInTotalSetVars */
      case 15: /* snmpInGetRequests */
      case 16: /* snmpInGetNexts */
      case 17: /* snmpInSetRequests */
      case 18: /* snmpInGetResponses */
      case 19: /* snmpInTraps */
      case 20: /* snmpOutTooBigs */
      case 21: /* snmpOutNoSuchNames */
      case 22: /* snmpOutBadValues */
      case 24: /* snmpOutGenErrs */
      case 25: /* snmpOutGetRequests */
      case 26: /* snmpOutGetNexts */
      case 27: /* snmpOutSetRequests */
      case 28: /* snmpOutGetResponses */
      case 29: /* snmpOutTraps */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_APPLIC | SNMP_ASN1_PRIMIT | SNMP_ASN1_COUNTER);
        od->v_len = sizeof(u32_t);
        break;
      case 30: /* snmpEnableAuthenTraps */
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_WRITE;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(s32_t);
        break;
      default:
        LWIP_DEBUGF(SNMP_MIB_DEBUG,("snmp_get_object_def: no such object"));
        od->instance = MIB_OBJECT_NONE;
        break;
    };
  }
  else
  {
    LWIP_DEBUGF(SNMP_MIB_DEBUG,("snmp_get_object_def: no scalar"));
    od->instance = MIB_OBJECT_NONE;
  }
}

static void
snmp_get_value(struct obj_def *od, u16_t len, void *value)
{
  u32_t *uint_ptr = value;
  u8_t id;

  if (len){}
  id = od->id_inst_ptr[0];
  switch (id)
  {
      case 1: /* snmpInPkts */
        *uint_ptr = snmpinpkts;
        break;
      case 2: /* snmpOutPkts */
        *uint_ptr = snmpoutpkts;
        break;
      case 3: /* snmpInBadVersions */
        *uint_ptr = snmpinbadversions;
        break;
      case 4: /* snmpInBadCommunityNames */
        *uint_ptr = snmpinbadcommunitynames;
        break;
      case 5: /* snmpInBadCommunityUses */
        *uint_ptr = snmpinbadcommunityuses;
        break;
      case 6: /* snmpInASNParseErrs */
        *uint_ptr = snmpinasnparseerrs;
        break;
      case 8: /* snmpInTooBigs */
        *uint_ptr = snmpintoobigs;
        break;
      case 9: /* snmpInNoSuchNames */
        *uint_ptr = snmpinnosuchnames;
        break;
      case 10: /* snmpInBadValues */
        *uint_ptr = snmpinbadvalues;
        break;
      case 11: /* snmpInReadOnlys */
        *uint_ptr = snmpinreadonlys;
        break;
      case 12: /* snmpInGenErrs */
        *uint_ptr = snmpingenerrs;
        break;
      case 13: /* snmpInTotalReqVars */
        *uint_ptr = snmpintotalreqvars;
        break;
      case 14: /* snmpInTotalSetVars */
        *uint_ptr = snmpintotalsetvars;
        break;
      case 15: /* snmpInGetRequests */
        *uint_ptr = snmpingetrequests;
        break;
      case 16: /* snmpInGetNexts */
        *uint_ptr = snmpingetnexts;
        break;
      case 17: /* snmpInSetRequests */
        *uint_ptr = snmpinsetrequests;
        break;
      case 18: /* snmpInGetResponses */
        *uint_ptr = snmpingetresponses;
        break;
      case 19: /* snmpInTraps */
        *uint_ptr = snmpintraps;
        break;
      case 20: /* snmpOutTooBigs */
        *uint_ptr = snmpouttoobigs;
        break;
      case 21: /* snmpOutNoSuchNames */
        *uint_ptr = snmpoutnosuchnames;
        break;
      case 22: /* snmpOutBadValues */
        *uint_ptr = snmpoutbadvalues;
        break;
      case 24: /* snmpOutGenErrs */
        *uint_ptr = snmpoutgenerrs;
        break;
      case 25: /* snmpOutGetRequests */
        *uint_ptr = snmpoutgetrequests;
        break;
      case 26: /* snmpOutGetNexts */
        *uint_ptr = snmpoutgetnexts;
        break;
      case 27: /* snmpOutSetRequests */
        *uint_ptr = snmpoutsetrequests;
        break;
      case 28: /* snmpOutGetResponses */
        *uint_ptr = snmpoutgetresponses;
        break;
      case 29: /* snmpOutTraps */
        *uint_ptr = snmpouttraps;
        break;
      case 30: /* snmpEnableAuthenTraps */
        *uint_ptr = *snmpenableauthentraps_ptr;
        break;
  };
}

#endif /* LWIP_SNMP */