if (stats_omnitalk_metadata.ok) {
	snprintf(statsbuffer, STATSBUFFER_SIZE, "omnitalk_metadata{esp_version=\"%s\",git_commit=\"%s\"} 1\n", stats_omnitalk_metadata.esp_version, stats_omnitalk_metadata.git_commit);
	httpd_resp_send_chunk(req, statsbuffer, HTTPD_RESP_USE_STRLEN);
}

