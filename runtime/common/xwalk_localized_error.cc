// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/common/xwalk_localized_error.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/error_page/common/net_error_info.h"
#include "grit/xwalk_resources.h"
#include "components/strings/grit/components_strings.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

using blink::WebURLError;

namespace {

struct LocalizedErrorMap {
  int error_code;
  unsigned int details_resource_id;
};

const LocalizedErrorMap net_error_options[] = {
  { net::ERR_TIMED_OUT,
    IDS_ERRORPAGES_DETAILS_TIMED_OUT,
  },
  { net::ERR_CONNECTION_TIMED_OUT,
    IDS_ERRORPAGES_DETAILS_TIMED_OUT,
  },
  { net::ERR_CONNECTION_CLOSED,
    IDS_ERRORPAGES_DETAILS_CONNECTION_CLOSED,
  },
  { net::ERR_CONNECTION_RESET,
    IDS_ERRORPAGES_DETAILS_CONNECTION_RESET,
  },
  { net::ERR_CONNECTION_REFUSED,
    IDS_ERRORPAGES_DETAILS_CONNECTION_REFUSED,
  },
  { net::ERR_CONNECTION_FAILED,
    IDS_ERRORPAGES_DETAILS_CONNECTION_FAILED,
  },
  { net::ERR_NAME_NOT_RESOLVED,
    IDS_ERRORPAGES_DETAILS_NAME_NOT_RESOLVED,
  },
  { net::ERR_ADDRESS_UNREACHABLE,
    IDS_ERRORPAGES_DETAILS_ADDRESS_UNREACHABLE,
  },
  { net::ERR_NETWORK_ACCESS_DENIED,
    IDS_ERRORPAGES_DETAILS_NETWORK_ACCESS_DENIED,
  },
  { net::ERR_PROXY_CONNECTION_FAILED,
    IDS_ERRORPAGES_DETAILS_PROXY_CONNECTION_FAILED,
  },
  { net::ERR_INTERNET_DISCONNECTED,
    IDS_ERRORPAGES_DETAILS_INTERNET_DISCONNECTED,
  },
  { net::ERR_FILE_NOT_FOUND,
    IDS_ERRORPAGES_DETAILS_FILE_NOT_FOUND,
  },
  { net::ERR_CACHE_MISS,
    IDS_ERRORPAGES_DETAILS_CACHE_MISS,
  },
  { net::ERR_CACHE_READ_FAILURE,
    IDS_ERRORPAGES_DETAILS_CACHE_READ_FAILURE,
  },
  { net::ERR_NETWORK_IO_SUSPENDED,
    IDS_ERRORPAGES_DETAILS_NETWORK_IO_SUSPENDED,
  },
  { net::ERR_TOO_MANY_REDIRECTS,
    IDS_ERRORPAGES_DETAILS_TOO_MANY_REDIRECTS,
  },
  { net::ERR_EMPTY_RESPONSE,
    IDS_ERRORPAGES_DETAILS_EMPTY_RESPONSE,
  },
  { net::ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_LENGTH,
    IDS_ERRORPAGES_DETAILS_RESPONSE_HEADERS_MULTIPLE_CONTENT_LENGTH,
  },
  { net::ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION,
    IDS_ERRORPAGES_DETAILS_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION,
  },
  { net::ERR_RESPONSE_HEADERS_MULTIPLE_LOCATION,
    IDS_ERRORPAGES_DETAILS_RESPONSE_HEADERS_MULTIPLE_LOCATION,
  },
  { net::ERR_CONTENT_LENGTH_MISMATCH,
    IDS_ERRORPAGES_DETAILS_CONNECTION_CLOSED,
  },
  { net::ERR_INCOMPLETE_CHUNKED_ENCODING,
    IDS_ERRORPAGES_DETAILS_CONNECTION_CLOSED,
  },
  { net::ERR_SSL_PROTOCOL_ERROR,
    IDS_ERRORPAGES_DETAILS_SSL_PROTOCOL_ERROR,
  },
  { net::ERR_BAD_SSL_CLIENT_AUTH_CERT,
    IDS_ERRORPAGES_DETAILS_BAD_SSL_CLIENT_AUTH_CERT,
  },
  { net::ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY,
    IDS_ERRORPAGES_DETAILS_SSL_PROTOCOL_ERROR,
  },
  { net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN,
    IDS_ERRORPAGES_DETAILS_PINNING_FAILURE,
  },
  { net::ERR_TEMPORARILY_THROTTLED,
    IDS_ERRORPAGES_DETAILS_TEMPORARILY_THROTTLED,
  },
  { net::ERR_BLOCKED_BY_CLIENT,
    IDS_ERRORPAGES_DETAILS_BLOCKED,
  },
  { net::ERR_NETWORK_CHANGED,
    IDS_ERRORPAGES_DETAILS_NETWORK_CHANGED,
  },
  { net::ERR_BLOCKED_BY_ADMINISTRATOR,
    IDS_ERRORPAGES_DETAILS_BLOCKED_BY_ADMINISTRATOR,
  },
};

// Special error page to be used in the case of navigating back to a page
// generated by a POST.  LocalizedError::HasStrings expects this net error code
// to also appear in the array above.

const LocalizedErrorMap repost_error = {
  net::ERR_CACHE_MISS,
  IDS_ERRORPAGES_DETAILS_CACHE_MISS,
};

const LocalizedErrorMap http_error_options[] = {
  { 403,
    IDS_ERRORPAGES_DETAILS_FORBIDDEN,
  },
  { 410,
    IDS_ERRORPAGES_DETAILS_GONE,
  },

  { 500,
    IDS_ERRORPAGES_DETAILS_INTERNAL_SERVER_ERROR,
  },
  { 501,
    IDS_ERRORPAGES_DETAILS_NOT_IMPLEMENTED,
  },
  { 502,
    IDS_ERRORPAGES_DETAILS_BAD_GATEWAY,
  },
  { 503,
    IDS_ERRORPAGES_DETAILS_SERVICE_UNAVAILABLE,
  },
  { 504,
    IDS_ERRORPAGES_DETAILS_GATEWAY_TIMEOUT,
  },
  { 505,
    IDS_ERRORPAGES_DETAILS_HTTP_VERSION_NOT_SUPPORTED,
  },
};

const LocalizedErrorMap dns_probe_error_options[] = {
  { error_page::DNS_PROBE_POSSIBLE,
    IDS_ERRORPAGES_DETAILS_DNS_PROBE_RUNNING,
  },

  // DNS_PROBE_NOT_RUN is not here; NetErrorHelper will restore the original
  // error, which might be one of several DNS-related errors.

  { error_page::DNS_PROBE_STARTED,
    IDS_ERRORPAGES_DETAILS_DNS_PROBE_RUNNING,
  },

  // DNS_PROBE_FINISHED_UNKNOWN is not here; NetErrorHelper will restore the
  // original error, which might be one of several DNS-related errors.

  { error_page::DNS_PROBE_FINISHED_NO_INTERNET,
    IDS_ERRORPAGES_DETAILS_INTERNET_DISCONNECTED,
  },
  { error_page::DNS_PROBE_FINISHED_BAD_CONFIG,
    IDS_ERRORPAGES_DETAILS_NAME_NOT_RESOLVED,
  },
  { error_page::DNS_PROBE_FINISHED_NXDOMAIN,
    IDS_ERRORPAGES_DETAILS_NAME_NOT_RESOLVED,
  },
};

const LocalizedErrorMap* FindErrorMapInArray(const LocalizedErrorMap* maps,
                                                   size_t num_maps,
                                                   int error_code) {
  for (size_t i = 0; i < num_maps; ++i) {
    if (maps[i].error_code == error_code)
      return &maps[i];
  }
  return NULL;
}

const LocalizedErrorMap* LookupErrorMap(const std::string& error_domain,
                                        int error_code, bool is_post) {
  if (error_domain == net::kErrorDomain) {
    // Display a different page in the special case of navigating through the
    // history to an uncached page created by a POST.
    if (is_post && error_code == net::ERR_CACHE_MISS)
      return &repost_error;
    return FindErrorMapInArray(net_error_options,
                               arraysize(net_error_options),
                               error_code);
  } else if (error_domain == LocalizedError::kHttpErrorDomain) {
    return FindErrorMapInArray(http_error_options,
                               arraysize(http_error_options),
                               error_code);
  } else if (error_domain == LocalizedError::kDnsProbeErrorDomain) {
    const LocalizedErrorMap* map =
        FindErrorMapInArray(dns_probe_error_options,
                            arraysize(dns_probe_error_options),
                            error_code);
    DCHECK(map);
    return map;
  } else {
    NOTREACHED();
    return NULL;
  }
}
}  // namespace

const char LocalizedError::kHttpErrorDomain[] = "http";
const char LocalizedError::kDnsProbeErrorDomain[] = "dnsprobe";

base::string16 LocalizedError::GetErrorDetails(const blink::WebURLError& error,
                                               bool is_post) {
  const LocalizedErrorMap* error_map =
      LookupErrorMap(error.domain.utf8(), error.reason, is_post);
  if (error_map)
    return l10n_util::GetStringUTF16(error_map->details_resource_id);
  else
    return l10n_util::GetStringUTF16(IDS_ERRORPAGES_DETAILS_UNKNOWN);
}
