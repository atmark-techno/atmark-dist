/* attr.c - backend routines for dealing with attributes */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-bdb/attr.c,v 1.8.2.5 2003/05/07 22:29:11 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-bdb.h"

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct bdb_attrinfo {
	AttributeDescription *ai_desc; /* attribute description cn;lang-en */
	slap_mask_t ai_indexmask;	/* how the attr is indexed	*/
} AttrInfo;

static int
ainfo_type_cmp(
	const void *v_desc,
	const void *v_a
)
{
	const AttributeDescription *desc = v_desc;
	const AttrInfo	*a = v_a;
	return SLAP_PTRCMP(desc, a->ai_desc);
}

static int
ainfo_cmp(
	const void	*v_a,
	const void	*v_b
)
{
	const AttrInfo *a = v_a, *b = v_b;
	return SLAP_PTRCMP(a->ai_desc, b->ai_desc);
}

void
bdb_attr_mask(
	struct bdb_info	*bdb,
	AttributeDescription *desc,
	slap_mask_t *indexmask )
{
	AttrInfo	*a;

	a = (AttrInfo *) avl_find( bdb->bi_attrs, desc, ainfo_type_cmp );
	
	*indexmask = a != NULL ? a->ai_indexmask : 0;
}

int
bdb_attr_index_config(
	struct bdb_info	*bdb,
	const char		*fname,
	int			lineno,
	int			argc,
	char		**argv )
{
	int rc;
	int	i;
	slap_mask_t mask;
	char **attrs;
	char **indexes = NULL;

	attrs = ldap_str2charray( argv[0], "," );

	if( attrs == NULL ) {
		fprintf( stderr, "%s: line %d: "
			"no attributes specified: %s\n",
			fname, lineno, argv[0] );
		return LDAP_PARAM_ERROR;
	}

	if ( argc > 1 ) {
		indexes = ldap_str2charray( argv[1], "," );

		if( indexes == NULL ) {
			fprintf( stderr, "%s: line %d: "
				"no indexes specified: %s\n",
				fname, lineno, argv[1] );
			return LDAP_PARAM_ERROR;
		}
	}

	if( indexes == NULL ) {
		mask = bdb->bi_defaultmask;

	} else {
		mask = 0;

		for ( i = 0; indexes[i] != NULL; i++ ) {
			slap_mask_t index;
			rc = slap_str2index( indexes[i], &index );

			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "%s: line %d: "
					"index type \"%s\" undefined\n",
					fname, lineno, indexes[i] );
				return LDAP_PARAM_ERROR;
			}

			mask |= index;
		}
	}

	if( !mask ) {
		fprintf( stderr, "%s: line %d: "
			"no indexes selected\n",
			fname, lineno );
		return LDAP_PARAM_ERROR;
	}

	for ( i = 0; attrs[i] != NULL; i++ ) {
		AttrInfo	*a;
		AttributeDescription *ad;
		const char *text;

		if( strcasecmp( attrs[i], "default" ) == 0 ) {
			bdb->bi_defaultmask = mask;
			continue;
		}

		a = (AttrInfo *) ch_malloc( sizeof(AttrInfo) );

		ad = NULL;
		rc = slap_str2ad( attrs[i], &ad, &text );

		if( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: line %d: "
				"index attribute \"%s\" undefined\n",
				fname, lineno, attrs[i] );
			return rc;
		}

		if( slap_ad_is_binary( ad ) ) {
			fprintf( stderr, "%s: line %d: "
				"index of attribute \"%s\" disallowed\n",
				fname, lineno, attrs[i] );
			return LDAP_UNWILLING_TO_PERFORM;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) && !(
			( ad->ad_type->sat_approx
				&& ad->ad_type->sat_approx->smr_indexer
				&& ad->ad_type->sat_approx->smr_filter )
			&& ( ad->ad_type->sat_equality
				&& ad->ad_type->sat_equality->smr_indexer
				&& ad->ad_type->sat_equality->smr_filter ) ) )
		{
			fprintf( stderr, "%s: line %d: "
				"approx index of attribute \"%s\" disallowed\n",
				fname, lineno, attrs[i] );
			return LDAP_INAPPROPRIATE_MATCHING;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) && !(
			ad->ad_type->sat_equality
				&& ad->ad_type->sat_equality->smr_indexer
				&& ad->ad_type->sat_equality->smr_filter ) )
		{
			fprintf( stderr, "%s: line %d: "
				"equality index of attribute \"%s\" disallowed\n",
				fname, lineno, attrs[i] );
			return LDAP_INAPPROPRIATE_MATCHING;
		}

		if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) && !(
			ad->ad_type->sat_substr
				&& ad->ad_type->sat_substr->smr_indexer
				&& ad->ad_type->sat_substr->smr_filter ) )
		{
			fprintf( stderr, "%s: line %d: "
				"substr index of attribute \"%s\" disallowed\n",
				fname, lineno, attrs[i] );
			return LDAP_INAPPROPRIATE_MATCHING;
		}

#ifdef NEW_LOGGING
		LDAP_LOG( BACK_BDB, DETAIL1, 
			"attr_index_config: index %s 0x%04lx\n",
			ad->ad_cname.bv_val, mask, 0 );
#else
		Debug( LDAP_DEBUG_CONFIG, "index %s 0x%04lx\n",
			ad->ad_cname.bv_val, mask, 0 ); 
#endif


		a->ai_desc = ad;
		a->ai_indexmask = mask;

		rc = avl_insert( &bdb->bi_attrs, (caddr_t) a,
		                 ainfo_cmp, avl_dup_error );

		if( rc ) {
			fprintf( stderr, "%s: line %d: duplicate index definition "
				"for attr \"%s\" (ignored)\n",
				fname, lineno, attrs[i] );

			return LDAP_PARAM_ERROR;
		}
	}

	ldap_charray_free( attrs );
	if ( indexes != NULL ) ldap_charray_free( indexes );

	return LDAP_SUCCESS;
}

void
bdb_attr_index_destroy( Avlnode *tree )
{
	avl_free( tree, free );
}

