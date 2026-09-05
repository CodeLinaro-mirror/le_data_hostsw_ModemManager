/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2025 Dan Williams <dan@ioncontrol.co>
 */

#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>

#define _LIBMM_INSIDE_MM
#include <libmm-glib.h>

#include "mm-sms-part-3gpp.h"
#include "mm-sms-list.h"
#include "mm-log-test.h"
#include "mm-base-modem.h"

#include "mm-iface-modem-messaging.h"

/****************************************************************/

static void
test_mbim_multipart_unstored (void)
{
    static const gchar *part1_pdu =
        "07912160130300f4440b915155685703f900005240713104738a3e050003c40202da6f37881e96"
        "9fcbf4b4fb0ccabfeb20f4fb0ea287e5e7323ded3e83dae17519747fcbd96490b95c6683d27310"
        "1d5d0601";
    static const gchar *part2_pdu =
        "07912160130300f4440b915155685703f900005240713104738aa0050003c40201ac69373d7c2e"
        "83e87538bc2cbf87e565d039dc2e83c220769a4e6797416132394d4fbfdda0fb5b4e4783c2ee3c"
        "888e2e83e86fd0db0c1a86e769f71b647eb3d9ef7bda7d06a5e7a0b09b0c9ab3df74109c1dce83"
        "e8e8301d44479741f9771d949e83e861f9b94c4fbbcf20f13b4c9f83e8e832485c068ddfedf6db"
        "0da2a3cba0fcbb0e1abfdb";
    MMSmsPart *part1, *part2;
    MMSmsList *list;
    MMBaseSms *sms1, *sms2;
    GError *error = NULL;

    /* Some MBIM devices (Dell 5821e) report SMSes via the MBIM_CID_SMS_READ
     * unsolicited notification usually used for Class-0 (flash/alert) messages.
     * These always have a message index of 0; thus when a multipart SMS
     * arrives it comes as two individual notifications both with 0 indexes.
     * The MBIM modem code used SMS_PART_INVALID_INDEX for unstored SMSes.
     * Ensure the MMSmsList can handle combining the two parts with the same
     * index.
     */
    part1 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, part1_pdu, NULL, &error);
    g_assert_no_error (error);
    part2 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, part2_pdu, NULL, &error);
    g_assert_no_error (error);

    list = mm_sms_list_new (NULL);

    sms1 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms1,
                           part1,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);

    sms2 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms2,
                           part2,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);

    g_assert_cmpint (mm_sms_list_get_count (list), ==, 1);
}

/****************************************************************/

static void
test_mbim_multipart_zero_index (void)
{
    static const gchar *pdu =
        "07912160130300f4440b915155685703f900005240713104738a3e050003c40202da6f37881e96"
        "9fcbf4b4fb0ccabfeb20f4fb0ea287e5e7323ded3e83dae17519747fcbd96490b95c6683d27310"
        "1d5d0601";
    MMSmsPart *part;
    MMSmsList *list;
    MMBaseSms *sms;
    GError *error = NULL;

    part = mm_sms_part_3gpp_new_from_pdu (0, pdu, NULL, &error);
    g_assert_no_error (error);

    list = mm_sms_list_new (NULL);

    sms = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                     MM_BASE_SMS_IS_3GPP, TRUE,
                                     MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                     NULL));
    mm_sms_list_take_part (list,
                           sms,
                           part,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);

    g_assert_cmpint (mm_sms_list_get_count (list), ==, 1);
}

/****************************************************************/

static void
test_multipart_match_pdu_type (void)
{
    MMSmsPart *part1, *part2;
    MMSmsList *list;
    MMBaseSms *sms1, *sms2;
    GError *error = NULL;

    static const gchar *deliver_part1_pdu =
        "07919471227210244405852122F039F101506271217180A005000319020198E9B2B82C0759DFE4"
        "B0F9ED2EB7967537B9CC02B5D37450122D2FCB41EE303DFD7687D96537881A96A7CD6F383DFD76"
        "83F46134BBEC064DD36550DA0D22A7CBF3721BE42CD3F5A0198B56036DCA20B8FC0D6A0A417076"
        "7D0EAAE540433A082E7F83A6E5F93CFD76BB40D7B2DB0D9AA6CB2072BA3C2F83926EF31BE44E8F"
        "D17450BB8C9683CA";
    static const gchar *deliver_part2_pdu =
        "07919471227210244405852122F039F1015062712181804F050003190202E4E8309B5E7683DAFC"
        "319A5E76B340F73D9A5D7683A6E93268FD9ED3CB6EF67B0E5AD172B19B2C2693C9602E90355D66"
        "83A6F0B007946E8382F5393BEC26BB00";

    static const gchar *submit_part1_pdu =
        "0791947122721024511905812122F000000BA005000319020198E9B2B82C0759DFE4B0F9ED2EB7"
        "967537B9CC02B5D37450122D2FCB41EE303DFD7687D96537881A96A7CD6F383DFD7683F46134BB"
        "EC064DD36550DA0D22A7CBF3721BE42CD3F5A0198B56036DCA20B8FC0D6A0A4170767D0EAAE540"
        "433A082E7F83A6E5F93CFD76BB40D7B2DB0D9AA6CB2072BA3C2F83926EF31BE44E8FD17450BB8C"
        "9683CA";

    static const gchar *submit_part2_pdu =
        "0791947122721024511905812122F000000B4F050003190202E4E8309B5E7683DAFC319A5E76B3"
        "40F73D9A5D7683A6E93268FD9ED3CB6EF67B0E5AD172B19B2C2693C9602E90355D6683A6F0B007"
        "946E8382F5393BEC26BB00";

    list = mm_sms_list_new (NULL);

    /* Add the initial SUBMIT PDU */
    part1 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, submit_part1_pdu, NULL, &error);
    g_assert_no_error (error);
    sms1 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms1,
                           part1,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);
    g_clear_object (&sms1);

    part2 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, submit_part2_pdu, NULL, &error);
    g_assert_no_error (error);
    sms2 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms2,
                           part2,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);
    g_clear_object (&sms2);

    g_assert_cmpint (mm_sms_list_get_count (list), ==, 1);

    /* Now take the DELIVER PDU */
    part1 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, deliver_part1_pdu, NULL, &error);
    g_assert_no_error (error);
    sms1 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms1,
                           part1,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);

    part2 = mm_sms_part_3gpp_new_from_pdu (SMS_PART_INVALID_INDEX, deliver_part2_pdu, NULL, &error);
    g_assert_no_error (error);
    sms2 = MM_BASE_SMS (g_object_new (MM_TYPE_BASE_SMS,
                                      MM_BASE_SMS_IS_3GPP, TRUE,
                                      MM_BASE_SMS_DEFAULT_STORAGE, MM_SMS_STORAGE_MT,
                                      NULL));
    mm_sms_list_take_part (list,
                           sms2,
                           part2,
                           MM_SMS_STATE_RECEIVED,
                           MM_SMS_STORAGE_MT,
                           &error);
    g_assert_no_error (error);

    /* Ensure we have two SMSs; the DELIVER and the SUBMIT */
    g_assert_cmpint (mm_sms_list_get_count (list), ==, 2);
    /* Ensure the DELIVER is complete */
    g_assert (mm_base_sms_is_multipart (sms1));
    g_assert (mm_base_sms_multipart_is_complete (sms1));
    /* sms2 should have no parts because sms1 took the second PDU */
    g_assert (!mm_base_sms_is_multipart (sms2));

    g_clear_object (&sms1);
    g_clear_object (&sms2);
    g_clear_object (&list);
}

/****************************************************************/

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");

    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/MM/SMS/3GPP/sms-list/zero-index", test_mbim_multipart_zero_index);
    g_test_add_func ("/MM/SMS/3GPP/sms-list/mbim-multipart-unstored", test_mbim_multipart_unstored);
    g_test_add_func ("/MM/SMS/3GPP/sms-list/multipart-match-pdu-type", test_multipart_match_pdu_type);

    return g_test_run ();
}
