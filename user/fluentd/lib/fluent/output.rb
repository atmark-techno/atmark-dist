#
# Fluentd
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

module Fluent
  class OutputChain
    def initialize(array, tag, es, chain=NullOutputChain.instance)
      @array = array
      @tag = tag
      @es = es
      @offset = 0
      @chain = chain
    end

    def next
      if @array.length <= @offset
        return @chain.next
      end
      @offset += 1
      result = @array[@offset-1].emit(@tag, @es, self)
      result
    end
  end

  class CopyOutputChain < OutputChain
    def next
      if @array.length <= @offset
        return @chain.next
      end
      @offset += 1
      es = @array.length > @offset ? @es.dup : @es
      result = @array[@offset-1].emit(@tag, es, self)
      result
    end
  end

  class NullOutputChain
    require 'singleton'
    include Singleton

    def next
    end
  end


  class Output
    include Configurable
    include PluginId
    include PluginLoggerMixin

    attr_accessor :router

    def initialize
      super
    end

    def configure(conf)
      super

      if label_name = conf['@label']
        label = Engine.root_agent.find_label(label_name)
        @router = label.event_router
      elsif @router.nil?
        @router = Engine.root_agent.event_router
      end
    end

    def start
    end

    def shutdown
    end

    #def emit(tag, es, chain)
    #end

    def secondary_init(primary)
      if primary.class != self.class
        $log.warn "type of secondary output should be same as primary output", :primary=>primary.class.to_s, :secondary=>self.class.to_s
      end
    end
  end


  class OutputThread
    def initialize(output)
      @output = output
      @finish = false
      @next_time = Engine.now + 1.0
    end

    def configure(conf)
    end

    def start
      @mutex = Mutex.new
      @cond = ConditionVariable.new
      @thread = Thread.new(&method(:run))
    end

    def shutdown
      @finish = true
      @mutex.synchronize {
        @cond.signal
      }
      Thread.pass
      @thread.join
    end

    def submit_flush
      @mutex.synchronize {
        @next_time = 0
        @cond.signal
      }
      Thread.pass
    end

    private
    def run
      @mutex.lock
      begin
        until @finish
          time = Engine.now

          if @next_time <= time
            @mutex.unlock
            begin
              @next_time = @output.try_flush
            ensure
              @mutex.lock
            end
            next_wait = @next_time - Engine.now
          else
            next_wait = @next_time - time
          end

          cond_wait(next_wait) if next_wait > 0
        end
      ensure
        @mutex.unlock
      end
    rescue
      $log.error "error on output thread", :error=>$!.to_s
      $log.error_backtrace
      raise
    ensure
      @mutex.synchronize {
        @output.before_shutdown
      }
    end

    def cond_wait(sec)
      @cond.wait(@mutex, sec)
    end
  end


  class BufferedOutput < Output
    def initialize
      super
      @next_flush_time = 0
      @last_retry_time = 0
      @next_retry_time = 0
      @num_errors = 0
      @num_errors_lock = Mutex.new
      @secondary_limit = 8
      @emit_count = 0
    end

    config_param :buffer_type, :string, :default => 'memory'
    config_param :flush_interval, :time, :default => 60
    config_param :try_flush_interval, :float, :default => 1
    config_param :disable_retry_limit, :bool, :default => false
    config_param :retry_limit, :integer, :default => 17
    config_param :retry_wait, :time, :default => 1.0
    config_param :max_retry_wait, :time, :default => nil
    config_param :num_threads, :integer, :default => 1
    config_param :queued_chunk_flush_interval, :time, :default => 1

    def configure(conf)
      super

      @buffer = Plugin.new_buffer(@buffer_type)
      @buffer.configure(conf)

      if @buffer.respond_to?(:enable_parallel)
        if @num_threads == 1
          @buffer.enable_parallel(false)
        else
          @buffer.enable_parallel(true)
        end
      end

      @writers = (1..@num_threads).map {
        writer = OutputThread.new(self)
        writer.configure(conf)
        writer
      }

      if sconf = conf.elements.select {|e| e.name == 'secondary' }.first
        type = sconf['@type'] || conf['@type'] || sconf['type'] || conf['type']
        @secondary = Plugin.new_output(type)
        @secondary.router = router
        @secondary.configure(sconf)

        if secondary_limit = conf['secondary_limit']
          @secondary_limit = secondary_limit.to_i
          if @secondary_limit < 0
            raise ConfigError, "invalid parameter 'secondary_limit #{secondary_limit}'"
          end
        end

        @secondary.secondary_init(self)
      end

      Status.register(self, "queue_size") { @buffer.queue_size }
      Status.register(self, "emit_count") { @emit_count }
    end

    def start
      @next_flush_time = Engine.now + @flush_interval
      @buffer.start
      @secondary.start if @secondary
      @writers.each {|writer| writer.start }
      @writer_current_position = 0
      @writers_size = @writers.size
    end

    def shutdown
      @writers.each {|writer| writer.shutdown }
      @secondary.shutdown if @secondary
      @buffer.shutdown
    end

    def emit(tag, es, chain, key="")
      @emit_count += 1
      data = format_stream(tag, es)
      if @buffer.emit(key, data, chain)
        submit_flush
      end
    end

    def submit_flush
      # Without locks: it is rough but enough to select "next" writer selection
      @writer_current_position = (@writer_current_position + 1) % @writers_size
      @writers[@writer_current_position].submit_flush
    end

    def format_stream(tag, es)
      out = ''
      es.each {|time,record|
        out << format(tag, time, record)
      }
      out
    end

    #def format(tag, time, record)
    #end

    #def write(chunk)
    #end

    def enqueue_buffer
      @buffer.keys.each {|key|
        @buffer.push(key)
      }
    end

    def try_flush
      time = Engine.now

      empty = @buffer.queue_size == 0
      if empty && @next_flush_time < (now = Engine.now)
        @buffer.synchronize do
          if @next_flush_time < now
            enqueue_buffer
            @next_flush_time = now + @flush_interval
            empty = @buffer.queue_size == 0
          end
        end
      end
      if empty
        return time + @try_flush_interval
      end

      begin
        retrying = !@num_errors.zero?

        if retrying
          @num_errors_lock.synchronize do
            if retrying = !@num_errors.zero? # re-check in synchronize
              if @next_retry_time >= time
                # allow retrying for only one thread
                return time + @try_flush_interval
              end
              # assume next retry failes and
              # clear them if when it succeeds
              @last_retry_time = time
              @num_errors += 1
              @next_retry_time += calc_retry_wait
            end
          end
        end

        if @secondary && !@disable_retry_limit && @num_errors > @retry_limit
          has_next = flush_secondary(@secondary)
        else
          has_next = @buffer.pop(self)
        end

        # success
        if retrying
          @num_errors = 0
          # Note: don't notify to other threads to prevent
          #       burst to recovered server
          $log.warn "retry succeeded.", :plugin_id=>plugin_id
        end

        if has_next
          return Engine.now + @queued_chunk_flush_interval
        else
          return time + @try_flush_interval
        end

      rescue => e
        if retrying
          error_count = @num_errors
        else
          # first error
          error_count = 0
          @num_errors_lock.synchronize do
            if @num_errors.zero?
              @last_retry_time = time
              @num_errors += 1
              @next_retry_time = time + calc_retry_wait
            end
          end
        end

        if @disable_retry_limit || error_count < @retry_limit
          $log.warn "temporarily failed to flush the buffer.", :next_retry=>Time.at(@next_retry_time), :error_class=>e.class.to_s, :error=>e.to_s, :plugin_id=>plugin_id
          $log.warn_backtrace e.backtrace

        elsif @secondary
          if error_count == @retry_limit
            $log.warn "failed to flush the buffer.", :error_class=>e.class.to_s, :error=>e.to_s, :plugin_id=>plugin_id
            $log.warn "retry count exceededs limit. falling back to secondary output."
            $log.warn_backtrace e.backtrace
            retry  # retry immediately
          elsif error_count <= @retry_limit + @secondary_limit
            $log.warn "failed to flush the buffer, next retry will be with secondary output.", :next_retry=>Time.at(@next_retry_time), :error_class=>e.class.to_s, :error=>e.to_s, :plugin_id=>plugin_id
            $log.warn_backtrace e.backtrace
          else
            $log.warn "failed to flush the buffer.", :error_class=>e.class, :error=>e.to_s, :plugin_id=>plugin_id
            $log.warn "secondary retry count exceededs limit."
            $log.warn_backtrace e.backtrace
            write_abort
            @num_errors = 0
          end

        else
          $log.warn "failed to flush the buffer.", :error_class=>e.class.to_s, :error=>e.to_s, :plugin_id=>plugin_id
          $log.warn "retry count exceededs limit."
          $log.warn_backtrace e.backtrace
          write_abort
          @num_errors = 0
        end

        return @next_retry_time
      end
    end

    def force_flush
      enqueue_buffer
      submit_flush
    end

    def before_shutdown
      begin
        @buffer.before_shutdown(self)
      rescue
        $log.warn "before_shutdown failed", :error=>$!.to_s
        $log.warn_backtrace
      end
    end

    def calc_retry_wait
      # TODO retry pattern
      wait = if @disable_retry_limit || @num_errors <= @retry_limit
               @retry_wait * (2 ** (@num_errors - 1))
             else
               # secondary retry
               @retry_wait * (2 ** (@num_errors - 2 - @retry_limit))
             end
      retry_wait = wait.finite? ? wait + (rand * (wait / 4.0) - (wait / 8.0)) : wait
      @max_retry_wait ? [retry_wait, @max_retry_wait].min : retry_wait
    end

    def write_abort
      $log.error "throwing away old logs."
      begin
        @buffer.clear!
      rescue
        $log.error "unexpected error while aborting", :error=>$!.to_s
        $log.error_backtrace
      end
    end

    def flush_secondary(secondary)
      @buffer.pop(secondary)
    end
  end


  class ObjectBufferedOutput < BufferedOutput
    def initialize
      super
    end

    def emit(tag, es, chain)
      @emit_count += 1
      data = es.to_msgpack_stream
      key = tag
      if @buffer.emit(key, data, chain)
        submit_flush
      end
    end

    module BufferedEventStreamMixin
      include Enumerable

      def repeatable?
        true
      end

      def each(&block)
        msgpack_each(&block)
      end

      def to_msgpack_stream
        read
      end
    end

    def write(chunk)
      chunk.extend(BufferedEventStreamMixin)
      write_objects(chunk.key, chunk)
    end
  end


  class TimeSlicedOutput < BufferedOutput
    require 'fluent/timezone'

    def initialize
      super
      @localtime = true
      #@ignore_old = false   # TODO
    end

    config_param :time_slice_format, :string, :default => '%Y%m%d'
    config_param :time_slice_wait, :time, :default => 10*60
    config_param :timezone, :string, :default => nil
    config_set_default :buffer_type, 'file'  # overwrite default buffer_type
    config_set_default :buffer_chunk_limit, 256*1024*1024  # overwrite default buffer_chunk_limit
    config_set_default :flush_interval, nil

    attr_accessor :localtime
    attr_reader :time_slicer # for test

    def configure(conf)
      super

      if conf['utc']
        @localtime = false
      elsif conf['localtime']
        @localtime = true
      end

      if conf['timezone']
        @timezone = conf['timezone']
        Fluent::Timezone.validate!(@timezone)
      end

      if @timezone
        @time_slicer = Timezone.formatter(@timezone, @time_slice_format)
      elsif @localtime
        @time_slicer = Proc.new {|time|
          Time.at(time).strftime(@time_slice_format)
        }
      else
        @time_slicer = Proc.new {|time|
          Time.at(time).utc.strftime(@time_slice_format)
        }
      end

      @time_slice_cache_interval = time_slice_cache_interval
      @before_tc = nil
      @before_key = nil

      if @flush_interval
        if conf['time_slice_wait']
          $log.warn "time_slice_wait is ignored if flush_interval is specified: #{conf}"
        end
        @enqueue_buffer_proc = Proc.new do
          @buffer.keys.each {|key|
            @buffer.push(key)
          }
        end

      else
        @flush_interval = [60, @time_slice_cache_interval].min
        @enqueue_buffer_proc = Proc.new do
          nowslice = @time_slicer.call(Engine.now.to_i - @time_slice_wait)
          @buffer.keys.each {|key|
            if key < nowslice
              @buffer.push(key)
            end
          }
        end
      end
    end

    def emit(tag, es, chain)
      @emit_count += 1
      es.each {|time,record|
        tc = time / @time_slice_cache_interval
        if @before_tc == tc
          key = @before_key
        else
          @before_tc = tc
          key = @time_slicer.call(time)
          @before_key = key
        end
        data = format(tag, time, record)
        if @buffer.emit(key, data, chain)
          submit_flush
        end
      }
    end

    def enqueue_buffer
      @enqueue_buffer_proc.call
    end

    #def format(tag, event)
    #end

    private
    def time_slice_cache_interval
      if @time_slicer.call(0) != @time_slicer.call(60-1)
        return 1
      elsif @time_slicer.call(0) != @time_slicer.call(60*60-1)
        return 30
      elsif @time_slicer.call(0) != @time_slicer.call(24*60*60-1)
        return 60*30
      else
        return 24*60*30
      end
    end
  end


  class MultiOutput < Output
    #def outputs
    #end
  end
end
