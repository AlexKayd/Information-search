-- public.documents определение

-- Drop table

-- DROP TABLE public.documents;

CREATE TABLE public.documents (
	id bigserial NOT NULL,
	url text NOT NULL,
	normalized_url text NOT NULL,
	"source" text NOT NULL,
	created_at timestamptz DEFAULT now() NULL,
	html text NULL,
	clean_text text NULL,
	title text NULL,
	author text NULL,
	publish_date text NULL,
	last_modified_header text NULL,
	etag_header text NULL,
	fetch_timestamp int8 NULL,
	last_fetch_attempt int8 NULL,
	CONSTRAINT documents_normalized_url_key UNIQUE (normalized_url),
	CONSTRAINT documents_pkey PRIMARY KEY (id)
);
CREATE INDEX idx_last_fetch ON public.documents USING btree (last_fetch_attempt);
CREATE UNIQUE INDEX idx_normalized_url ON public.documents USING btree (normalized_url);